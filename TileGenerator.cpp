/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  23.08.2012 12:35:53
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#include <cstdio>
#include <cstdlib>
#include <climits>
#include <fstream>
#include <gdfontmb.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include "config.h"
#include "PlayerAttributes.h"
#include "TileGenerator.h"
#include "ZlibDecompressor.h"
#if USE_SQLITE3
#include "db-sqlite3.h"
#endif
#if USE_LEVELDB
#include "db-leveldb.h"
#endif
#if USE_REDIS
#include "db-redis.h"
#endif

using namespace std;

static inline int64_t pythonmodulo(int64_t i, int64_t mod)
{
	if (i >= 0) {
		return i % mod;
	}
	else {
		return mod - ((-i) % mod);
	}
}

static inline int unsignedToSigned(long i, long max_positive)
{
	if (i < max_positive) {
		return i;
	}
	else {
		return i - 2l * max_positive;
	}
}

static inline uint16_t readU16(const unsigned char *data)
{
	return data[0] << 8 | data[1];
}

static inline int readBlockContent(const unsigned char *mapData, int version, int datapos)
{
	if (version >= 24) {
		size_t index = datapos << 1;
		return (mapData[index] << 8) | mapData[index + 1];
	}
	else if (version >= 20) {
		if (mapData[datapos] <= 0x80) {
			return mapData[datapos];
		}
		else {
			return (int(mapData[datapos]) << 4) | (int(mapData[datapos + 0x2000]) >> 4);
		}
	}
	else {
		std::ostringstream oss;
		oss << "Unsupported map version " << version;
		throw std::runtime_error(oss.str());
	}
}

TileGenerator::TileGenerator():
	verboseCoordinates(0),
	verboseStatistics(false),
	progressIndicator(false),
	m_bgColor(255, 255, 255),
	m_scaleColor(0, 0, 0),
	m_originColor(255, 0, 0),
	m_playerColor(255, 0, 0),
	m_tileBorderColor(0, 0, 0),
	m_drawOrigin(false),
	m_drawPlayers(false),
	m_drawScale(false),
	m_drawAlpha(false),
	m_shading(true),
	m_border(0),
	m_backend(DEFAULT_BACKEND),
	m_shrinkGeometry(true),
	m_blockGeometry(false),
	m_sqliteCacheWorldRow(false),
	m_image(0),
	m_xMin(INT_MAX/16-1),
	m_xMax(INT_MIN/16+1),
	m_zMin(INT_MAX/16-1),
	m_zMax(INT_MIN/16+1),
	m_yMin(INT_MAX/16-1),
	m_yMax(INT_MIN/16+1),
	m_reqXMin(MAPBLOCK_MIN),
	m_reqXMax(MAPBLOCK_MAX),
	m_reqYMin(MAPBLOCK_MIN),
	m_reqYMax(MAPBLOCK_MAX),
	m_reqZMin(MAPBLOCK_MIN),
	m_reqZMax(MAPBLOCK_MAX),
	m_reqYMinNode(0),
	m_reqYMaxNode(15),
	m_mapXStartNodeOffset(0),
	m_mapYStartNodeOffset(0),
	m_mapXEndNodeOffset(0),
	m_mapYEndNodeOffset(0),
	m_nextStoredYCoord(0),
	m_tileXOrigin(TILECENTER_IS_WORLDCENTER),
	m_tileZOrigin(TILECENTER_IS_WORLDCENTER),
	m_tileWidth(0),
	m_tileHeight(0),
	m_tileBorderSize(1),
	m_tileMapXOffset(0),
	m_tileMapYOffset(0)
{
}

TileGenerator::~TileGenerator()
{
}

void TileGenerator::setBgColor(const Color &bgColor)
{
	m_bgColor = bgColor;
}

void TileGenerator::setShrinkGeometry(bool shrink)
{
	m_shrinkGeometry = shrink;
}

void TileGenerator::setBlockGeometry(bool block)
{
	m_blockGeometry = block;
}

void TileGenerator::setSqliteCacheWorldRow(bool cacheWorldRow)
{
	m_sqliteCacheWorldRow = cacheWorldRow;
}

void TileGenerator::setScaleColor(const Color &scaleColor)
{
	m_scaleColor = scaleColor;
}

void TileGenerator::setOriginColor(const Color &originColor)
{
	m_originColor = originColor;
}

void TileGenerator::setPlayerColor(const Color &playerColor)
{
	m_playerColor = playerColor;
}

void TileGenerator::setTileBorderColor(const Color &tileBorderColor)
{
	m_tileBorderColor = tileBorderColor;
}

void TileGenerator::setTileBorderSize(int size)
{
	m_tileBorderSize = size;
}

void TileGenerator::setTileSize(int width, int heigth)
{
	m_tileWidth = width;
	m_tileHeight = heigth;
}

void TileGenerator::setTileOrigin(int x, int y)
{
	m_tileXOrigin = x;
	m_tileZOrigin = y;
}

void TileGenerator::setDrawOrigin(bool drawOrigin)
{
	m_drawOrigin = drawOrigin;
}

void TileGenerator::setDrawPlayers(bool drawPlayers)
{
	m_drawPlayers = drawPlayers;
}

void TileGenerator::setDrawScale(bool drawScale)
{
	m_drawScale = drawScale;
	if (m_drawScale) {
		m_border = 40;
	}
}

void TileGenerator::setDrawAlpha(bool drawAlpha)
{
    m_drawAlpha = drawAlpha;
}

void TileGenerator::setShading(bool shading)
{
	m_shading = shading;
}

void TileGenerator::enableProgressIndicator(void)
{
	progressIndicator = true;
}

void TileGenerator::setGeometry(int x, int y, int w, int h)
{
	if (x > 0) {
		m_reqXMin = x / 16;
	}
	else {
		m_reqXMin = (x - 15) / 16;
	}
	if (y > 0) {
		m_reqZMin = y / 16;
	}
	else {
		m_reqZMin = (y - 15) / 16;
	}
	m_mapXStartNodeOffset = x - m_reqXMin * 16;
	m_mapYEndNodeOffset = m_reqZMin * 16 - y;

	int x2 = x + w - 1;
	int y2 = y + h - 1;

	if (x2 > 0) {
		m_reqXMax = x2 / 16;
	}
	else {
		m_reqXMax = (x2 - 15) / 16;
	}
	if (y2 > 0) {
		m_reqZMax = y2 / 16;
	}
	else {
		m_reqZMax = (y2 - 15) / 16;
	}
	m_mapXEndNodeOffset = x2 - (m_reqXMax * 16 + 15);
	m_mapYStartNodeOffset = (m_reqZMax * 16 + 15) - y2;
}

void TileGenerator::setMinY(int y)
{
	if (y > 0) {
		m_reqYMin = y / 16;
	}
	else {
		m_reqYMin = (y - 15) / 16;
	}
	m_reqYMinNode = y - 16 * m_reqYMin;
}

void TileGenerator::setMaxY(int y)
{
	if (y > 0) {
		m_reqYMax = y / 16;
	}
	else {
		m_reqYMax = (y - 15) / 16;
	}
	m_reqYMaxNode = y - 16 * m_reqYMax;
}

void TileGenerator::parseColorsFile(const std::string &fileName)
{
	ifstream in;
	in.open(fileName.c_str(), ifstream::in);
	if (!in.is_open()) {
		throw std::runtime_error(std::string("Failed to open colors file '") + fileName + "'");
		return;
	}
	parseColorsStream(in, fileName.c_str());
}

void TileGenerator::setBackend(std::string backend)
{
	m_backend = backend;
}

void TileGenerator::generate(const std::string &input, const std::string &output)
{
	string input_path = input;
	if (input_path[input.length() - 1] != PATH_SEPARATOR) {
		input_path += PATH_SEPARATOR;
	}

	openDb(input_path);
	loadBlocks();
	computeMapParameters();
	createImage();
	renderMap();
	if (m_drawScale) {
		renderScale();
	}
	if (m_drawOrigin) {
		renderOrigin();
	}
	if (m_drawPlayers) {
		renderPlayers(input_path);
	}
	writeImage(output);
	printUnknown();
}

void TileGenerator::parseColorsStream(std::istream &in, const std::string &filename)
{
	string line;
	int linenr = 0;
	for (std::getline(in,line); in.good(); std::getline(in,line)) {
		linenr++;
		istringstream iline;
		iline.str(line);
		iline >> std::skipws;
		string name;
		ColorEntry color;
		iline >> name;
		if (name.length() == 0 || name[0] == '#')
			continue;
		int r, g, b, a, t;
		iline >> r;
		iline >> g;
		iline >> b;
		if (!iline.good() && !iline.eof()) {
			std::cerr << filename << ":" << linenr << ": bad line in colors file (" << line << ")" << std::endl;
			continue;
		}
		a = 0xff;
		iline >> a;
		t = 0;
		iline >> t;
		color = ColorEntry(r,g,b,a,t);
		if ((m_drawAlpha && a == 0xff) || (!m_drawAlpha && a != 0xff)) {
			// If drawing alpha, and the colors file contains both
			// an opaque entry and a non-opaque entry for a name, prefer
			// the non-opaque entry
			// If not drawing alpha, and the colors file contains both
			// an opaque entry and a non-opaque entry for a name, prefer
			// the opaque entry
			// Otherwise, any later entry overrides any previous entry
			ColorMap::iterator it = m_colors.find(name);
			if (it != m_colors.end()) {
				if (m_drawAlpha && (a == 0xff && it->second.a != 0xff)) {
					// drawing alpha: don't use opaque color to override
					// non-opaque color
					continue;
				}
				if (!m_drawAlpha && (a != 0xff && it->second.a == 0xff)) {
					// not drawing alpha: don't use non-opaque color to
					// override opaque color
					continue;
				}
			}
		}
		m_colors[name] = color;
	}
	if (!in.eof()) {
		std::cerr << filename << ": error reading colors file after line " << linenr << std::endl;
	}
}

std::string TileGenerator::getWorldDatabaseBackend(const std::string &input)
{
	string backend;

	std::string worldFile = input + PATH_SEPARATOR + "world.mt";
	ifstream in;
	in.open(worldFile.c_str(), ifstream::in);
	if (!in.is_open())
		throw std::runtime_error(std::string("Failed to open world.mt file '") + worldFile + "'");

	std::string line;
	int linenr = 0;
	for (std::getline(in,line); in.good(); std::getline(in,line)) {
		linenr++;
		istringstream iline;
		iline.str(line);
		iline >> std::skipws;
		string variable;
		string eq;
		iline >> variable;
		if (variable != "backend")
			continue;
		iline >> eq;
		iline >> backend;
		if (in.fail() || eq != "=") {
			ostringstream oss;
			oss << "Error parsing 'backend' in world.mt file at line " << linenr;
			throw std::runtime_error(oss.str());
		}
	}
	in.close();
	if (backend == "")
		backend = "sqlite3";
	return backend;
}

void TileGenerator::openDb(const std::string &input)
{
	string backend = m_backend;
	bool unsupported = false;
	if (m_backend == "auto")
		backend = getWorldDatabaseBackend(input);

	if(backend == "sqlite3") {
#if USE_SQLITE3
		DBSQLite3 *db;
		m_db = db = new DBSQLite3(input);
		db->cacheWorldRow = m_sqliteCacheWorldRow;
#else
		unsupported = true;
#endif
	}
	else if (backend == "leveldb") {
#if USE_LEVELDB
		m_db = new DBLevelDB(input);
#else
		unsupported = true;
#endif
	}
	else if (backend == "redis") {
#if USE_REDIS
		m_db = new DBRedis(input);
#else
		unsupported = true;
#endif
	}
	else if (m_backend == "auto")
		throw std::runtime_error(((std::string) "World uses unrecognised database backend: ") + backend);
	else
		throw std::runtime_error(((std::string) "Internal error: unknown database backend: ") + m_backend);

	if (unsupported)
		throw std::runtime_error(((std::string) "World uses backend '") + backend + ", which was not enabled at compile-time.");
}

void TileGenerator::loadBlocks()
{
	#define MESSAGE_WIDTH 25
	int mapXMin, mapXMax;
	int mapYMin, mapYMax;
	int mapZMin, mapZMax;
	int geomYMin, geomYMax;
	long long world_blocks;
	long long map_blocks;
	if (verboseCoordinates >= 2) {
		bool partialBlocks = (m_mapXStartNodeOffset || m_mapXEndNodeOffset || m_mapYStartNodeOffset || m_mapYEndNodeOffset);
		if (partialBlocks || !m_blockGeometry) {
			cout
				<< std::setw(MESSAGE_WIDTH) << std::left
				<< (m_blockGeometry ? "Command-line Geometry:" : "Requested Geometry:")
				<< std::right
				<< std::setw(7) << m_reqXMin*16+m_mapXStartNodeOffset << ","
				<< std::setw(7) << m_reqYMin*16+m_reqYMinNode << ","
				<< std::setw(7) << m_reqZMin*16-m_mapYEndNodeOffset
				<< "  ..  "
				<< std::setw(7) << m_reqXMax*16+15+m_mapXEndNodeOffset << ","
				<< std::setw(7) << m_reqYMax*16+m_reqYMaxNode << ","
				<< std::setw(7) << m_reqZMax*16+15-m_mapYStartNodeOffset
				<< "    ("
				<< std::setw(6) << m_reqXMin << ","
				<< std::setw(6) << m_reqYMin << ","
				<< std::setw(6) << m_reqZMin
				<< "  ..  "
				<< std::setw(6) << m_reqXMax << ","
				<< std::setw(6) << m_reqYMax << ","
				<< std::setw(6) << m_reqZMax
				<< ")\n";
		}
		if (partialBlocks || m_blockGeometry) {
			cout
				<< std::setw(MESSAGE_WIDTH) << std::left
				<< (m_blockGeometry ? "Requested Geometry:" : "Block-aligned Geometry:")
				<< std::right
				<< std::setw(7) << m_reqXMin*16 << ","
				<< std::setw(7) << m_reqYMin*16+m_reqYMinNode << ","
				<< std::setw(7) << m_reqZMin*16 <<
				"  ..  "
				<< std::setw(7) << m_reqXMax*16+15 << ","
				<< std::setw(7) << m_reqYMax*16+m_reqYMaxNode << ","
				<< std::setw(7) << m_reqZMax*16+15
				<< "    ("
				<< std::setw(6) << m_reqXMin << ","
				<< std::setw(6) << m_reqYMin << ","
				<< std::setw(6) << m_reqZMin
				<< "  ..  "
				<< std::setw(6) << m_reqXMax << ","
				<< std::setw(6) << m_reqYMax << ","
				<< std::setw(6) << m_reqZMax
				<< ")\n";
		}
	}
	if (m_blockGeometry) {
		m_mapXStartNodeOffset = 0;
		m_mapXEndNodeOffset = 0;
		m_mapYStartNodeOffset = 0;
		m_mapYEndNodeOffset = 0;
	}
	mapXMin = INT_MAX/16-1;
	mapXMax = -INT_MIN/16+1;
	mapYMin = INT_MAX/16-1;
	mapYMax = -INT_MIN/16+1;
	mapZMin = INT_MAX/16-1;
	mapZMax = -INT_MIN/16+1;
	geomYMin = INT_MAX/16-1;
	geomYMax = -INT_MIN/16+1;
	const DB::BlockPosList &blocks = m_db->getBlockPos();
	world_blocks = 0;
	map_blocks = 0;
	for(DB::BlockPosList::const_iterator it = blocks.begin(); it != blocks.end(); ++it) {
		world_blocks ++;
		BlockPos pos = *it;
		if (pos.x < mapXMin) {
			mapXMin = pos.x;
		}
		if (pos.x > mapXMax) {
			mapXMax = pos.x;
		}
		if (pos.y < mapYMin) {
			mapYMin = pos.y;
		}
		if (pos.y > mapYMax) {
			mapYMax = pos.y;
		}
		if (pos.z < mapZMin) {
			mapZMin = pos.z;
		}
		if (pos.z > mapZMax) {
			mapZMax = pos.z;
		}
		if (pos.x < m_reqXMin || pos.x > m_reqXMax || pos.z < m_reqZMin || pos.z > m_reqZMax) {
			continue;
		}
		if (pos.y < geomYMin) {
			geomYMin = pos.y;
		}
		if (pos.y > geomYMax) {
			geomYMax = pos.y;
		}
		if (pos.y < m_reqYMin || pos.y > m_reqYMax) {
			continue;
		}
		map_blocks++;
		if (pos.y < m_yMin) {
			m_yMin = pos.y;
		}
		if (pos.y > m_yMax) {
			m_yMax = pos.y;
		}
		if (pos.x < m_xMin) {
			m_xMin = pos.x;
		}
		if (pos.x > m_xMax) {
			m_xMax = pos.x;
		}
		if (pos.z < m_zMin) {
			m_zMin = pos.z;
		}
		if (pos.z > m_zMax) {
			m_zMax = pos.z;
		}
		m_positions.push_back(pos);
	}
	if (verboseCoordinates >= 1) {
		cout
			<< std::setw(MESSAGE_WIDTH) << std::left
			<< "World Geometry:" << std::right
			<< std::setw(7) << mapXMin*16 << ","
			<< std::setw(7) << mapYMin*16 << ","
			<< std::setw(7) << mapZMin*16
			<< "  ..  "
			<< std::setw(7) << mapXMax*16+15 << ","
			<< std::setw(7) << mapYMax*16+15 << ","
			<< std::setw(7) << mapZMax*16+15
			<< "    ("
			<< std::setw(6) << mapXMin << ","
			<< std::setw(6) << mapYMin << ","
			<< std::setw(6) << mapZMin
			<< "  ..  "
			<< std::setw(6) << mapXMax << ","
			<< std::setw(6) << mapYMax << ","
			<< std::setw(6) << mapZMax
			<< ")    blocks: "
			<< std::setw(10) << world_blocks << "\n";
	}
	if (m_shrinkGeometry) {
		if (m_xMin != m_reqXMin) m_mapXStartNodeOffset = 0;
		if (m_xMax != m_reqXMax) m_mapXEndNodeOffset = 0;
		if (m_zMin != m_reqZMin) m_mapYEndNodeOffset = 0;
		if (m_zMax != m_reqZMax) m_mapYStartNodeOffset = 0;
	}
	else {
		if (verboseCoordinates >= 2) {
			cout
				<< std::setw(MESSAGE_WIDTH) << std::left
				<< "Minimal Map Geometry:" << std::right
				<< std::setw(7) << m_xMin*16 << ","
				<< std::setw(7) << m_yMin*16+m_reqYMinNode << ","
				<< std::setw(7) << m_zMin*16
				<< "  ..  "
				<< std::setw(7) << m_xMax*16+15 << ","
				<< std::setw(7) << m_yMax*16+m_reqYMaxNode << ","
				<< std::setw(7) << m_zMax*16+15
				<< "    ("
				<< std::setw(6) << m_xMin << ","
				<< std::setw(6) << m_yMin << ","
				<< std::setw(6) << m_zMin
				<< "  ..  "
				<< std::setw(6) << m_xMax << ","
				<< std::setw(6) << m_yMax << ","
				<< std::setw(6) << m_zMax
				<< ")\n";
		}
		m_xMin = m_reqXMin;
		m_xMax = m_reqXMax;
		m_zMin = m_reqZMin;
		m_zMax = m_reqZMax;
	}
	if (verboseCoordinates >= 2) {
		cout
			<< std::setw(MESSAGE_WIDTH) << std::left
			<< "Map Vertical Limits:" << std::right
			<< std::setw(7) << "x" << ","
			<< std::setw(7) << geomYMin*16 << ","
			<< std::setw(7) << "z"
			<< "  ..  "
			<< std::setw(7) << "x" << ","
			<< std::setw(7) << geomYMax*16+15 << ","
			<< std::setw(7) << "z"
			<< "    ("
			<< std::setw(6) << "x" << ","
			<< std::setw(6) << geomYMin << ","
			<< std::setw(6) << "z"
			<< "  ..  "
			<< std::setw(6) << "x" << ","
			<< std::setw(6) << geomYMax << ","
			<< std::setw(6) << "z"
			<< ")\n";
	}
	if (verboseCoordinates >= 1) {
		cout
			<< std::setw(MESSAGE_WIDTH) << std::left
			<< "Map Output Geometry:" << std::right
			<< std::setw(7) << m_xMin*16+m_mapXStartNodeOffset << ","
			<< std::setw(7) << m_yMin*16+m_reqYMinNode << ","
			<< std::setw(7) << m_zMin*16-m_mapYEndNodeOffset
			<< "  ..  "
			<< std::setw(7) << m_xMax*16+15+m_mapXEndNodeOffset << ","
			<< std::setw(7) << m_yMax*16+m_reqYMaxNode << ","
			<< std::setw(7) << m_zMax*16+15-m_mapYStartNodeOffset
			<< "    ("
			<< std::setw(6) << m_xMin << ","
			<< std::setw(6) << m_yMin << ","
			<< std::setw(6) << m_zMin
			<< "  ..  "
			<< std::setw(6) << m_xMax << ","
			<< std::setw(6) << m_yMax << ","
			<< std::setw(6) << m_zMax
			<< ")    blocks: "
			<< std::setw(10) << map_blocks << "\n";
	}
	m_positions.sort();
	#undef MESSAGE_WIDTH
}

inline BlockPos TileGenerator::decodeBlockPos(int64_t blockId) const
{
	BlockPos pos;
	pos.x = unsignedToSigned(pythonmodulo(blockId, 4096), 2048);
	blockId = (blockId - pos.x) / 4096;
	pos.y = unsignedToSigned(pythonmodulo(blockId, 4096), 2048);
	blockId = (blockId - pos.y) / 4096;
	pos.z = unsignedToSigned(pythonmodulo(blockId, 4096), 2048);
	return pos;
}

void TileGenerator::pushPixelRows(int zPosLimit) {
	if (m_shading)
		m_blockPixelAttributes.renderShading(m_drawAlpha);
	int y;
	for (y = m_nextStoredYCoord; y <= m_blockPixelAttributes.getLastY() && y < worldBlockZ2StoredY(m_zMin - 1) + m_mapYEndNodeOffset; y++) {
		for (int x = m_mapXStartNodeOffset; x < worldBlockX2StoredX(m_xMax + 1) + m_mapXEndNodeOffset; x++) {
			int mapX = x - m_mapXStartNodeOffset;
			int mapY = y - m_mapYStartNodeOffset;
			PixelAttribute &pixel = m_blockPixelAttributes.attribute(y, x);
#ifdef DEBUG
			{ int ix = mapX2ImageX(mapX); assert(ix - m_border >= 0 && ix - m_border < m_pictWidth); }
			{ int iy = mapY2ImageY(mapY); assert(iy - m_border >= 0 && iy - m_border < m_pictHeight); }
#endif
			if (pixel.is_valid())
				m_image->tpixels[mapY2ImageY(mapY)][mapX2ImageX(mapX)] = pixel.color().to_libgd();
		}
	}
	int yLimit = worldBlockZ2StoredY(zPosLimit);
	if (y <= yLimit) {
		m_blockPixelAttributes.scroll(yLimit);
		m_nextStoredYCoord = yLimit;
	}
}

void TileGenerator::computeTileParameters(
		// Input parameters
		int minPos,
		int maxPos,
		int mapStartNodeOffset,
		int mapEndNodeOffset,
		int tileOrigin,
		int tileSize,
		// Input / Output parameters
		int &pictSize,
		// Output parameters
		int &tileBorderCount,
		int &tileMapStartOffset,
		int &tileMapEndOffset,
		// Behavior selection
		bool ascending)
{
	int start = minPos * 16 + mapStartNodeOffset - tileOrigin;
	int limit = (maxPos+1) * 16 + mapEndNodeOffset - tileOrigin;
	int shift;
	// shift values, so that start = 0..tileSize-1
	// (effect of tileOrigin is identical to (tileOrigin + tileSize)
	//  so any multiple of tileSize can be safely added)
	if (start<0)
		shift = - (start + 1) / tileSize + 1;
	else
		shift = - start / tileSize;
	start += shift * tileSize;
	limit += shift * tileSize;

	int tileBorderStart = 0;	// First border to draw
	int tileBorderLimit = 0;	// Last + 1 border to draw
	if (ascending) {
		// Prefer tile borders towards negative infinity
		// 0 -> 0
		// 1..tileSize -> 1
		// (tileSize+1)..(2*tileSize) -> 2
		// etc.
		tileBorderStart = (start + tileSize - 1) / tileSize;
		tileBorderLimit = (limit + tileSize - 1) / tileSize;
	} else {
		// Prefer tile borders towards positive infinity
		// 0..(tileSize-1) -> 1
		// tileSize..(2*tileSize-1) -> 2
		// etc.
		tileBorderStart = start / tileSize + 1;
		tileBorderLimit = limit / tileSize + 1;
	}
	tileMapStartOffset = (tileSize - start) % tileSize;
	tileMapEndOffset = limit - ((tileBorderLimit-tileBorderStart) * tileSize);
	pictSize += (tileBorderLimit - tileBorderStart) * m_tileBorderSize;
	tileBorderCount = tileBorderLimit - tileBorderStart;
}

void TileGenerator::computeMapParameters()
{
	m_storedWidth = (m_xMax - m_xMin + 1) * 16;
	m_storedHeight = (m_zMax - m_zMin + 1) * 16;
	int mapWidth = m_storedWidth - m_mapXStartNodeOffset + m_mapXEndNodeOffset;
	int mapHeight = m_storedHeight - m_mapYStartNodeOffset + m_mapYEndNodeOffset;
	m_blockPixelAttributes.setParameters(m_storedWidth, 16);

	// Set special values for origin (which depend on other paramters)
	if (m_tileWidth) {
		if (m_tileXOrigin == TILECENTER_IS_WORLDCENTER)
			m_tileXOrigin = -m_tileWidth/2;
		else if (m_tileXOrigin == TILECENTER_IS_MAPCENTER)
			//m_tileXOrigin = ((m_xMax+1)*2-(m_xMax+1-m_xMin))*8 - m_tileWidth/2;
			m_tileXOrigin = m_xMin * 16 + m_mapXStartNodeOffset + mapWidth / 2;
	}
	if (m_tileHeight) {
		if (m_tileZOrigin == TILECENTER_IS_WORLDCENTER)
			m_tileZOrigin = -m_tileHeight/2;
		else if (m_tileZOrigin == TILECENTER_IS_MAPCENTER)
			//m_tileZOrigin = ((m_zMax+1)*2-(m_zMax+1-m_zMin))*8 - m_tileHeight/2;
			m_tileZOrigin = -m_xMax * 16 + m_mapYStartNodeOffset + mapHeight / 2;
	}

	// Compute adjustments for tiles.
	m_pictWidth = mapWidth;
	m_pictHeight = mapHeight;
	if (m_tileWidth && m_tileBorderSize) {
		int tileMapXEndOffset;		// Dummy
		TileGenerator::computeTileParameters(
				// Input parameters
				m_xMin,
				m_xMax,
				m_mapXStartNodeOffset,
				m_mapXEndNodeOffset,
				m_tileXOrigin,
				m_tileWidth,
				// Input / Output parameters
				m_pictWidth,
				// Output parameters
				m_tileBorderXCount,
				m_tileMapXOffset,
				tileMapXEndOffset,
				// Behavior selection
				true);
	}
	if (m_tileHeight && m_tileBorderSize) {
		int tileMapYEndOffset;		// Dummy
		TileGenerator::computeTileParameters(
				// Input parameters
				m_zMin,
				m_zMax,
				m_mapYEndNodeOffset,
				m_mapYStartNodeOffset,
				m_tileZOrigin,
				m_tileHeight,
				// Input / Output parameters
				m_pictHeight,
				// Output parameters
				m_tileBorderYCount,
				tileMapYEndOffset,
				m_tileMapYOffset,
				// Behavior selection
				false);
	}

	m_nextStoredYCoord = m_mapYStartNodeOffset;

	// Print some useful messages in cases where it may not be possible to generate the image...
	long long pixels = static_cast<long long>(m_pictWidth + m_border) * (m_pictHeight + m_border);
	// Study the libgd code to known why the maximum is the following:
	long long max_pixels = INT_MAX - INT_MAX % m_pictHeight;
	if (pixels > max_pixels) {
		cerr << "WARNING: Image will have " << pixels << " pixels; the PNG graphics library will refuse to handle more than approximately " << INT_MAX << std::endl;
	}
	// Estimated approximate maximum was determined by trial and error...
	// (24100x24100 succeeded; 24200x24200 failed)
	#define ESTIMATED_MAX_PIXELS_32BIT (24100*24100L)
	else if (sizeof(void *) == 4 && pixels > ESTIMATED_MAX_PIXELS_32BIT) {
		cerr << "WARNING: Image will have " << pixels << " pixels; The maximum achievable on a 32-bit system is approximately " << ESTIMATED_MAX_PIXELS_32BIT << std::endl;
	}
	#undef ESTIMATED_MAX_PIXELS_32BIT
}


void TileGenerator::createImage()
{
	m_image = gdImageCreateTrueColor(m_pictWidth + m_border, m_pictHeight + m_border);
	if (!m_image) {
		ostringstream oss;
		oss << "Failed to allocate " << m_pictWidth + m_border << "x" << m_pictHeight + m_border << " image";
		throw std::runtime_error(oss.str());
	}
	// Background
	gdImageFilledRectangle(m_image, 0, 0, m_pictWidth + m_border - 1, m_pictHeight + m_border -1, m_bgColor.to_libgd());

	// Draw tile borders
	if (m_tileWidth && m_tileBorderSize) {
		int borderColor = m_tileBorderColor.to_libgd();
		for (int i = 0; i < m_tileBorderXCount; i++) {
			int xPos = m_tileMapXOffset + i * (m_tileWidth + m_tileBorderSize);
#ifdef DEBUG
			int xPos2 = mapX2ImageX(m_tileMapXOffset + i * m_tileWidth) - m_border - 1;
			assert(xPos == xPos2);
#endif
			gdImageFilledRectangle(m_image, xPos + m_border, m_border, xPos + (m_tileBorderSize-1) + m_border, m_pictHeight + m_border - 1, borderColor);
		}
	}
	if (m_tileHeight && m_tileBorderSize) {
		int borderColor = m_tileBorderColor.to_libgd();
		for (int i = 0; i < m_tileBorderYCount; i++) {
			int yPos = m_tileMapYOffset + i * (m_tileHeight + m_tileBorderSize);
#ifdef DEBUG
			int yPos2 = mapY2ImageY(m_tileMapYOffset + i * m_tileHeight) - m_border - 1;
			assert(yPos == yPos2);
#endif
			gdImageFilledRectangle(m_image, m_border, yPos + m_border, m_pictWidth + m_border - 1, yPos + (m_tileBorderSize-1) + m_border, borderColor);
		}
	}
}

void TileGenerator::renderMap()
{
	int blocks_rendered = 0;
	int area_rendered = 0;
	BlockPos currentPos;
	currentPos.x = INT_MIN;
	currentPos.y = 0;
	currentPos.z = INT_MIN;
	bool allReaded = false;
	for (std::list<BlockPos>::const_iterator position = m_positions.begin(); position != m_positions.end(); ++position) {
		const BlockPos &pos = *position;
		if (currentPos.x != pos.x || currentPos.z != pos.z) {
			area_rendered++;
			if (currentPos.z != pos.z) {
				pushPixelRows(pos.z);
				m_blockPixelAttributes.setLastY((m_zMax - pos.z) * 16 + 15);
				if (progressIndicator)
				    cout << "Processing Z-coordinate: " << std::setw(5) << pos.z*16 << "\r" << std::flush;
			}
			for (int i = 0; i < 16; ++i) {
				m_readedPixels[i] = 0;
			}
			allReaded = false;
			currentPos = pos;
		}
		else if (allReaded) {
			continue;
		}
		DB::Block block = m_db->getBlockOnPos(pos);
		if (!block.second.empty()) {
			const unsigned char *data = block.second.c_str();
			size_t length = block.second.length();

			uint8_t version = data[0];
			//uint8_t flags = data[1];

			size_t dataOffset = 0;
			if (version >= 22) {
				dataOffset = 4;
			}
			else {
				dataOffset = 2;
			}

			ZlibDecompressor decompressor(data, length);
			decompressor.setSeekPos(dataOffset);
			ustring mapData = decompressor.decompress();
			ustring mapMetadata = decompressor.decompress();
			dataOffset = decompressor.seekPos();

			// Skip unused data
			if (version <= 21) {
				dataOffset += 2;
			}
			if (version == 23) {
				dataOffset += 1;
			}
			if (version == 24) {
				uint8_t ver = data[dataOffset++];
				if (ver == 1) {
					uint16_t num = readU16(data + dataOffset);
					dataOffset += 2;
					dataOffset += 10 * num;
				}
			}

			// Skip unused static objects
			dataOffset++; // Skip static object version
			int staticObjectCount = readU16(data + dataOffset);
			dataOffset += 2;
			for (int i = 0; i < staticObjectCount; ++i) {
				dataOffset += 13;
				uint16_t dataSize = readU16(data + dataOffset);
				dataOffset += dataSize + 2;
			}
			dataOffset += 4; // Skip timestamp

			m_blockAirId = -1;
			m_blockIgnoreId = -1;
			// Read mapping
			if (version >= 22) {
				dataOffset++; // mapping version
				uint16_t numMappings = readU16(data + dataOffset);
				dataOffset += 2;
				for (int i = 0; i < numMappings; ++i) {
					uint16_t nodeId = readU16(data + dataOffset);
					dataOffset += 2;
					uint16_t nameLen = readU16(data + dataOffset);
					dataOffset += 2;
					string name = string(reinterpret_cast<const char *>(data) + dataOffset, nameLen);
					name = name.c_str();		// Truncate any trailing NUL bytes
					if (name == "air") {
						m_blockAirId = nodeId;
					}
					else if (name == "ignore") {
						m_blockIgnoreId = nodeId;
					}
					else {
						m_nameMap[nodeId] = name;
					}
					dataOffset += nameLen;
				}
			}

			// Node timers
			if (version >= 25) {
				dataOffset++;
				uint16_t numTimers = readU16(data + dataOffset);
				dataOffset += 2;
				dataOffset += numTimers * 10;
			}

			renderMapBlock(mapData, pos, version);
			blocks_rendered++;

			allReaded = true;
			for (int i = 0; i < 16; ++i) {
				if (m_readedPixels[i] != 0xffff) {
					allReaded = false;
				}
			}
		}
	}
	if (currentPos.z != INT_MIN)
		pushPixelRows(currentPos.z - 1);
	if (verboseStatistics)
		cout << "Statistics"
		     << ":  blocks read: " << m_db->getBlocksReadCount()
		     << "  (" << m_db->getBlocksCachedCount() << " cached + "
		               << m_db->getBlocksUnCachedCount() << " uncached)"
		     << ";  blocks rendered: " << blocks_rendered
		     << ";  area rendered: " << area_rendered
		     << "/" << (m_xMax-m_xMin+1) * (m_zMax-m_zMin+1)
		     << "  (" << (long long)area_rendered*16*16 << " nodes)"
		     << std::endl;
	else if (progressIndicator)
		cout << std::setw(40) << "" << "\r";
}

inline void TileGenerator::renderMapBlock(const ustring &mapBlock, const BlockPos &pos, int version)
{
	int xBegin = worldBlockX2StoredX(pos.x);
	int zBegin = worldBlockZ2StoredY(pos.z);
	const unsigned char *mapData = mapBlock.c_str();
	int minY = (pos.y < m_reqYMin) ? 16 : (pos.y > m_reqYMin) ?  0 : m_reqYMinNode;
	int maxY = (pos.y > m_reqYMax) ? -1 : (pos.y < m_reqYMax) ? 15 : m_reqYMaxNode;
	for (int z = 0; z < 16; ++z) {
		for (int x = 0; x < 16; ++x) {
			if (m_readedPixels[z] & (1 << x)) {
				continue;
			}
			for (int y = maxY; y >= minY; --y) {
				int position = x + (y << 4) + (z << 8);
				int content = readBlockContent(mapData, version, position);
				if (content == m_blockIgnoreId || content == m_blockAirId) {
					continue;
				}
				NodeID2NameMap::iterator blockName = m_nameMap.find(content);
				if (blockName == m_nameMap.end())
					continue;
				const string &name = blockName->second;
				ColorMap::const_iterator color = m_colors.find(name);
				if (color != m_colors.end()) {
					PixelAttribute pixel = PixelAttribute(color->second, pos.y * 16 + y);
					if (m_drawAlpha) {
						m_blockPixelAttributes.attribute(zBegin + 15 - z,xBegin + x).mixUnder(pixel);
						if(pixel.alpha() == 0xff) {
							m_readedPixels[z] |= (1 << x);
							break;
						}
					} else {
						m_blockPixelAttributes.attribute(zBegin + 15 - z, xBegin + x) = pixel;
						m_readedPixels[z] |= (1 << x);
						break;
					}
				} else {
					m_unknownNodes.insert(name);
				}
			}
		}
	}
}

void TileGenerator::renderScale()
{
	int color = m_scaleColor.to_libgd();
	gdImageString(m_image, gdFontGetMediumBold(), 24, 0, reinterpret_cast<unsigned char *>(const_cast<char *>("X")), color);
	gdImageString(m_image, gdFontGetMediumBold(), 2, 24, reinterpret_cast<unsigned char *>(const_cast<char *>("Z")), color);

	string scaleText;

	for (int i = (m_xMin / 4) * 4; i <= m_xMax; i += 4) {
		stringstream buf;
		buf << i * 16;
		scaleText = buf.str();

		int xPos = worldX2ImageX(i * 16);
		gdImageString(m_image, gdFontGetMediumBold(), xPos + 2, 0, reinterpret_cast<unsigned char *>(const_cast<char *>(scaleText.c_str())), color);
		gdImageLine(m_image, xPos, 0, xPos, m_border - 1, color);
	}

	for (int i = (m_zMax / 4) * 4; i >= m_zMin; i -= 4) {
		stringstream buf;
		buf << i * 16;
		scaleText = buf.str();

		int yPos = worldZ2ImageY(i * 16);
		gdImageString(m_image, gdFontGetMediumBold(), 2, yPos, reinterpret_cast<unsigned char *>(const_cast<char *>(scaleText.c_str())), color);
		gdImageLine(m_image, 0, yPos, m_border - 1, yPos, color);
	}
}

void TileGenerator::renderOrigin()
{
	int imageX = worldX2ImageX(0);
	int imageY = worldZ2ImageY(0);
	gdImageArc(m_image, imageX, imageY, 12, 12, 0, 360, m_originColor.to_libgd());
}

void TileGenerator::renderPlayers(const std::string &inputPath)
{
	int color = m_playerColor.to_libgd();

	PlayerAttributes players(inputPath);
	for (PlayerAttributes::Players::iterator player = players.begin(); player != players.end(); ++player) {
		int imageX = worldX2ImageX(player->x / 10);
		int imageY = worldZ2ImageY(player->z / 10);

		gdImageArc(m_image, imageX, imageY, 5, 5, 0, 360, color);
		gdImageString(m_image, gdFontGetMediumBold(), imageX + 2, imageY + 2, reinterpret_cast<unsigned char *>(const_cast<char *>(player->name.c_str())), color);
	}
}

inline std::list<int> TileGenerator::getZValueList() const
{
	std::list<int> zlist;
	for (std::list<BlockPos>::const_iterator position = m_positions.begin(); position != m_positions.end(); ++position) {
		zlist.push_back(position->z);
	}
	zlist.sort();
	zlist.unique();
	zlist.reverse();
	return zlist;
}

void TileGenerator::writeImage(const std::string &output)
{
	FILE *out;
	out = fopen(output.c_str(), "wb");
	if (!out) {
		std::ostringstream oss;
		oss << "Error opening '" << output.c_str() << "': " << std::strerror(errno);
		throw std::runtime_error(oss.str());
	}
	gdImagePng(m_image, out);
	fclose(out);
	gdImageDestroy(m_image);
}

void TileGenerator::printUnknown()
{
	if (m_unknownNodes.size() > 0) {
		std::cerr << "Unknown nodes:" << std::endl;
		for (std::set<std::string>::iterator node = m_unknownNodes.begin(); node != m_unknownNodes.end(); ++node) {
			std::cerr << *node << std::endl;
		}
	}
}

// Adjust map coordinate for tiles and border
inline int TileGenerator::mapX2ImageX(int val) const
{
	if (m_tileWidth && m_tileBorderSize)
		val += ((val - m_tileMapXOffset + m_tileWidth) / m_tileWidth) * m_tileBorderSize + m_border;
	else
		val += m_border;
	return val;
}

// Adjust map coordinate for tiles and border
inline int TileGenerator::mapY2ImageY(int val) const
{
	if (m_tileHeight && m_tileBorderSize)
		val += ((val - m_tileMapYOffset + m_tileHeight) / m_tileHeight) * m_tileBorderSize + m_border;
	else
		val += m_border;
	return val;
}

// Convert world coordinate to image coordinate
inline int TileGenerator::worldX2ImageX(int val) const
{
	val = (val - m_xMin * 16) - m_mapXStartNodeOffset;
	return mapX2ImageX(val);
}

// Convert world coordinate to image coordinate
inline int TileGenerator::worldZ2ImageY(int val) const
{
	val = (m_zMax * 16 + 15 - val) - m_mapYStartNodeOffset;
	return mapY2ImageY(val);
}

