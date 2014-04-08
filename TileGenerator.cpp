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
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include "config.h"
#include "PlayerAttributes.h"
#include "TileGenerator.h"
#include "ZlibDecompressor.h"
#include "colors.h"
#include "db-sqlite3.h"
#if USE_LEVELDB
#include "db-leveldb.h"
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

static inline int rgb2int(uint8_t r, uint8_t g, uint8_t b)
{
	return (r << 16) + (g << 8) + b;
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

static inline int colorSafeBounds(int color)
{
	if (color > 255) {
		return 255;
	}
	else if (color < 0) {
		return 0;
	}
	else {
		return color;
	}
}

TileGenerator::TileGenerator():
	verboseCoordinates(false),
	verboseStatistics(false),
	m_bgColor(255, 255, 255),
	m_scaleColor(0, 0, 0),
	m_originColor(255, 0, 0),
	m_playerColor(255, 0, 0),
	m_tileBorderColor(0, 0, 0),
	m_drawOrigin(false),
	m_drawPlayers(false),
	m_drawScale(false),
	m_shading(true),
	m_border(0),
	m_backend("sqlite3"),
	m_forceGeom(false),
	m_sqliteCacheWorldRow(false),
	m_image(0),
	m_xMin(INT_MAX/16-1),
	m_xMax(INT_MIN/16+1),
	m_zMin(INT_MAX/16-1),
	m_zMax(INT_MIN/16+1),
	m_yMin(INT_MAX/16-1),
	m_yMax(INT_MIN/16+1),
	m_reqXMin(MINETEST_MAPBLOCK_MIN),
	m_reqXMax(MINETEST_MAPBLOCK_MAX),
	m_reqYMin(MINETEST_MAPBLOCK_MIN),
	m_reqYMax(MINETEST_MAPBLOCK_MAX),
	m_reqZMin(MINETEST_MAPBLOCK_MIN),
	m_reqZMax(MINETEST_MAPBLOCK_MAX),
	m_reqYMinNode(0),
	m_reqYMaxNode(15),
	m_tileXOrigin(TILECENTER_IS_WORLDCENTER),
	m_tileZOrigin(TILECENTER_IS_WORLDCENTER),
	m_tileWidth(0),
	m_tileHeight(0),
	m_tileBorderSize(1),
	m_tileMapXOffset(0),
	m_tileMapYOffset(0)
{
	string colors_txt_data(reinterpret_cast<char *>(colors_txt), colors_txt_len);
	istringstream colors_stream(colors_txt_data);
	parseColorsStream(colors_stream);
}

TileGenerator::~TileGenerator()
{
}

void TileGenerator::setBgColor(const std::string &bgColor)
{
	m_bgColor = parseColor(bgColor);
}

void TileGenerator::setForceGeom(bool forceGeom)
{
	m_forceGeom = forceGeom;
}

void TileGenerator::setSqliteCacheWorldRow(bool cacheWorldRow)
{
	m_sqliteCacheWorldRow = cacheWorldRow;
}

void TileGenerator::setScaleColor(const std::string &scaleColor)
{
	m_scaleColor = parseColor(scaleColor);
}

void TileGenerator::setOriginColor(const std::string &originColor)
{
	m_originColor = parseColor(originColor);
}

void TileGenerator::setPlayerColor(const std::string &playerColor)
{
	m_playerColor = parseColor(playerColor);
}

void TileGenerator::setTileBorderColor(const std::string &tileBorderColor)
{
	m_tileBorderColor = parseColor(tileBorderColor);
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

Color TileGenerator::parseColor(const std::string &color)
{
	Color parsed;
	if (color.length() != 7) {
		throw std::runtime_error("Color not 7 characters long");
	}
	if (color[0] != '#') {
		throw std::runtime_error("Color does not begin with #");
	}
	long col = strtol(color.c_str() + 1, NULL, 16);
	parsed.b = col % 256;
	col = col / 256;
	parsed.g = col % 256;
	col = col / 256;
	parsed.r = col % 256;
	return parsed;
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

void TileGenerator::setShading(bool shading)
{
	m_shading = shading;
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
		return;
	}
	parseColorsStream(in);
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

void TileGenerator::parseColorsStream(std::istream &in)
{
	while (in.good()) {
		string name;
		Color color;
		in >> name;
		if (name[0] == '#') {
			in.ignore(65536, '\n');
			in >> name;
		}
		while (name == "\n" && in.good()) {
			in >> name;
		}
		int r, g, b;
		in >> r;
		in >> g;
		in >> b;
		if (in.good()) {
			color.r = r;
			color.g = g;
			color.b = b;
			m_colors[name] = color;
		}
	}
}

void TileGenerator::openDb(const std::string &input)
{
	if(m_backend == "sqlite3") {
		DBSQLite3 *db;
		m_db = db = new DBSQLite3(input);
		db->cacheWorldRow = m_sqliteCacheWorldRow;
	}
#if USE_LEVELDB
	else if(m_backend == "leveldb")
		m_db = new DBLevelDB(input);
#endif
	else
		throw std::runtime_error(((std::string) "Unknown map backend: ") + m_backend);
}

void TileGenerator::loadBlocks()
{
	int mapXMin, mapXMax;
	int mapYMin, mapYMax;
	int mapZMin, mapZMax;
	int geomYMin, geomYMax;
	long long world_blocks;
	long long map_blocks;
	if (verboseCoordinates) {
		cout << "Requested Geometry:   "
			<< m_reqXMin*16 << "," << m_reqYMin*16+m_reqYMinNode << "," << m_reqZMin*16 << ".."
			<< m_reqXMax*16+15 << "," << m_reqYMax*16+m_reqYMaxNode << "," << m_reqZMax*16+15
			<< "    ("
			<< m_reqXMin << "," << m_reqYMin << "," << m_reqZMin << ".."
			<< m_reqXMax << "," << m_reqYMax << "," << m_reqZMax
			<< ")\n";
	}
	mapXMin = INT_MAX/16-1;
	mapXMax = -INT_MIN/16+1;
	mapYMin = INT_MAX/16-1;
	mapYMax = -INT_MIN/16+1;
	mapZMin = INT_MAX/16-1;
	mapZMax = -INT_MIN/16+1;
	geomYMin = INT_MAX/16-1;
	geomYMax = -INT_MIN/16+1;
	std::vector<int64_t> vec = m_db->getBlockPos();
	world_blocks = 0;
	map_blocks = 0;
	for(std::vector<int64_t>::iterator it = vec.begin(); it != vec.end(); ++it) {
		world_blocks ++;
		BlockPos pos = decodeBlockPos(*it);
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
	if (verboseCoordinates) {
		cout << "World Geometry:       "
			<< mapXMin*16 << "," << mapYMin*16 << "," << mapZMin*16 << ".."
			<< mapXMax*16+15 << "," << mapYMax*16+15 << "," << mapZMax*16+15
			<< "    ("
			<< mapXMin << "," << mapYMin << "," << mapZMin << ".."
			<< mapXMax << "," << mapYMax << "," << mapZMax
			<< ")    blocks: " << world_blocks << "\n";
	}
	if (m_forceGeom) {
		if (verboseCoordinates) {
			cout << "Minimal Map Geometry: "
				<< m_xMin*16 << "," << m_yMin*16+m_reqYMinNode << "," << m_zMin*16 << ".."
				<< m_xMax*16+15 << "," << m_yMax*16+m_reqYMaxNode << "," << m_zMax*16+15
				<< "    ("
				<< m_xMin << "," << m_yMin << "," << m_zMin << ".."
				<< m_xMax << "," << m_yMax << "," << m_zMax
				<< ")\n";
		}
		m_xMin = m_reqXMin;
		m_xMax = m_reqXMax;
		m_zMin = m_reqZMin;
		m_zMax = m_reqZMax;
	}
	if (verboseCoordinates) {
		cout << "Map Vertical Limits:  x," << geomYMin*16 << ",z..x," << geomYMax*16+15 << ",z    (x," <<   geomYMin << ",z..x," <<   geomYMax << ",z)\n";
		cout << "Map Output Geometry:  "
			<< m_xMin*16 << "," << m_yMin*16+m_reqYMinNode << "," << m_zMin*16 << ".."
			<< m_xMax*16+15 << "," << m_yMax*16+m_reqYMaxNode << "," << m_zMax*16+15
			<< "    ("
			<< m_xMin << "," << m_yMin << "," << m_zMin << ".."
			<< m_xMax << "," << m_yMax << "," << m_zMax
			<< ")    blocks: " << map_blocks << "\n";
	}
	m_positions.sort();
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

void TileGenerator::createImage()
{
	m_mapWidth = (m_xMax - m_xMin + 1) * 16;
	m_mapHeight = (m_zMax - m_zMin + 1) * 16;
	m_blockPixelAttributes.setWidth(m_mapWidth);

	// Set special values for origin (which depend on other paramters)
	if (m_tileWidth) {
		if (m_tileXOrigin == TILECENTER_IS_WORLDCENTER)
			m_tileXOrigin = -m_tileWidth/2;
		else if (m_tileXOrigin == TILECENTER_IS_MAPCENTER)
			m_tileXOrigin = ((m_xMax+1)*2-(m_xMax+1-m_xMin))*8 - m_tileWidth/2;
	}
	if (m_tileHeight) {
		if (m_tileZOrigin == TILECENTER_IS_WORLDCENTER)
			m_tileZOrigin = -m_tileHeight/2;
		else if (m_tileZOrigin == TILECENTER_IS_MAPCENTER)
			m_tileZOrigin = ((m_zMax+1)*2-(m_zMax+1-m_zMin))*8 - m_tileHeight/2;
	}

	int pictWidth = m_mapWidth;
	int pictHeight = m_mapHeight;
	int tileBorderXStart = 0;
	int tileBorderXLimit = 0;
	int tileBorderZStart = 0;
	int tileBorderZLimit = 0;
	if (m_tileWidth && m_tileBorderSize) {
		int xStart = m_xMin * 16 - m_tileXOrigin;
		int xLimit = (m_xMax+1) * 16 - m_tileXOrigin;
		int shift;
		// shift values, so that xStart = 0..m_tileWidth-1
		// (effect of m_tileXOrigin is identical to (m_tileXOrigin + m_tileWidth)
		//  so any multiple of m_tileWidth can be safely added)
		if (xStart<0)
			shift = - (xStart + 1) / m_tileWidth + 1;
		else
			shift = - xStart / m_tileWidth;
		xStart += shift * m_tileWidth;
		xLimit += shift * m_tileWidth;

		// 0 -> 0
		// 1..m_tileWidth -> 1
		// (m_tileWidth+1)..(2*m_tileWidth) -> 2
		// etc.
		tileBorderXStart = (xStart + m_tileWidth - 1) / m_tileWidth;
		tileBorderXLimit = (xLimit + m_tileWidth - 1) / m_tileWidth;
		m_tileMapXOffset = (m_tileWidth - xStart) % m_tileWidth;
		pictWidth += (tileBorderXLimit - tileBorderXStart) * m_tileBorderSize;
	}
	if (m_tileHeight && m_tileBorderSize) {
		int zStart = m_zMin * 16 - m_tileZOrigin;
		int zLimit = (m_zMax+1) * 16 - m_tileZOrigin;
		int shift;
		// shift values so that zStart = 0..m_tileHeight-1
		if (zStart<0)
			shift = - (zStart + 1) / m_tileHeight + 1;
		else
			shift = - zStart / m_tileHeight;
		zStart += shift * m_tileHeight;
		zLimit += shift * m_tileHeight;

		// 0..(m_tileWidth-1) -> 1
		// m_tileWidth..(2*m_tileWidth-1) -> 2
		// etc.
		tileBorderZStart = zStart / m_tileHeight + 1;
		tileBorderZLimit = zLimit / m_tileHeight + 1;
		m_tileMapYOffset = zLimit - ((tileBorderZLimit-tileBorderZStart) * m_tileHeight);
		pictHeight += (tileBorderZLimit - tileBorderZStart) * m_tileBorderSize;
	}

	m_image = gdImageCreateTrueColor(pictWidth + m_border, pictHeight + m_border);
	// Background
	gdImageFilledRectangle(m_image, 0, 0, pictWidth + m_border - 1, pictHeight + m_border -1, rgb2int(m_bgColor.r, m_bgColor.g, m_bgColor.b));

	// Draw tile borders
	if (m_tileWidth && m_tileBorderSize) {
		int borderColor = rgb2int(m_tileBorderColor.r, m_tileBorderColor.g, m_tileBorderColor.b);
		for (int i = 0; i < tileBorderXLimit - tileBorderXStart; i++) {
			int xPos = m_tileMapXOffset + i * (m_tileWidth + m_tileBorderSize);
			gdImageFilledRectangle(m_image, xPos + m_border, m_border, xPos + (m_tileBorderSize-1) + m_border, pictHeight + m_border - 1, borderColor);
		}
	}
	if (m_tileHeight && m_tileBorderSize) {
		int borderColor = rgb2int(m_tileBorderColor.r, m_tileBorderColor.g, m_tileBorderColor.b);
		for (int i = 0; i < tileBorderZLimit - tileBorderZStart; i++) {
			int yPos = m_tileMapYOffset + i * (m_tileHeight + m_tileBorderSize);
			gdImageFilledRectangle(m_image, m_border, yPos + m_border, pictWidth + m_border - 1, yPos + (m_tileBorderSize-1) + m_border, borderColor);
		}
	}
}

TileGenerator::Block TileGenerator::getBlockOnPos(BlockPos pos)
{
	DBBlock in = m_db->getBlockOnPos(pos.x, pos.y, pos.z);
	Block out(pos,(const unsigned char *)"");

	if (!in.second.empty()) {
		out = Block(decodeBlockPos(in.first), in.second);
		// Verify. Just to be sure...
		if (pos.x != out.first.x || pos.y != out.first.y || pos.z != out.first.z) {
			std::ostringstream oss;
			oss << "Got unexpexted block: "
			    << out.first.x << "," << out.first.y << "," << out.first.z
			    << " from database. Requested: "
			    << pos.x << "," << pos.y << "," << pos.z;
			throw std::runtime_error(oss.str());
		}
	}
	else {
		// I can't imagine this to be possible...
		std::ostringstream oss;
		oss << "Failed to get block: "
		    << pos.x << "," << pos.y << "," << pos.z
		    << " from database.";
		throw std::runtime_error(oss.str());
	}
	return out;
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
			if (currentPos.z != pos.z && currentPos.z != INT_MIN && m_shading)
				renderShading(currentPos.z);
			for (int i = 0; i < 16; ++i) {
				m_readedPixels[i] = 0;
			}
			currentPos = pos;
		}
		else if (allReaded) {
			continue;
		}
		Block block = getBlockOnPos(pos);
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
			ZlibDecompressor::string mapData = decompressor.decompress();
			ZlibDecompressor::string mapMetadata = decompressor.decompress();
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
	if(currentPos.z != INT_MIN && m_shading)
		renderShading(currentPos.z);
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
}

inline void TileGenerator::renderMapBlock(const unsigned_string &mapBlock, const BlockPos &pos, int version)
{
	int xBegin = (pos.x - m_xMin) * 16;
	int zBegin = (m_zMax - pos.z) * 16;
	const unsigned char *mapData = mapBlock.c_str();
	int minY = (pos.y < m_reqYMin) ? 16 : (pos.y > m_reqYMin) ?  0 : m_reqYMinNode;
	int maxY = (pos.y > m_reqYMax) ? -1 : (pos.y < m_reqYMax) ? 15 : m_reqYMaxNode;
	for (int z = 0; z < 16; ++z) {
		int imageY = getImageY(zBegin + 15 - z);
		for (int x = 0; x < 16; ++x) {
			if (m_readedPixels[z] & (1 << x)) {
				continue;
			}
			int imageX = getImageX(xBegin + x);
			for (int y = maxY; y >= minY; --y) {
				int position = x + (y << 4) + (z << 8);
				int content = readBlockContent(mapData, version, position);
				if (content == m_blockIgnoreId || content == m_blockAirId) {
					continue;
				}
				std::map<int, std::string>::iterator blockName = m_nameMap.find(content);
				if (blockName != m_nameMap.end()) {
					const string &name = blockName->second;
					ColorMap::const_iterator color = m_colors.find(name);
					if (color != m_colors.end()) {
						const Color &c = color->second;
						m_image->tpixels[imageY][imageX] = rgb2int(c.r, c.g, c.b);
						m_readedPixels[z] |= (1 << x);
						m_blockPixelAttributes.attribute(15 - z, xBegin + x).height = pos.y * 16 + y;
					} else {
						m_unknownNodes.insert(name);
						continue;
					}
					break;
				}
			}
		}
	}
}

inline void TileGenerator::renderShading(int zPos)
{
	int zBegin = (m_zMax - zPos) * 16;
	for (int z = 0; z < 16; ++z) {
		int imageY = zBegin + z;
		if (imageY >= m_mapHeight) {
			continue;
		}
		imageY = getImageY(imageY);
		for (int x = 0; x < m_mapWidth; ++x) {
			if (!m_blockPixelAttributes.attribute(z, x).valid_height() || !m_blockPixelAttributes.attribute(z, x - 1).valid_height() || !m_blockPixelAttributes.attribute(z - 1, x).valid_height()) {
				continue;
			}
			int y = m_blockPixelAttributes.attribute(z, x).height;
			int y1 = m_blockPixelAttributes.attribute(z, x - 1).height;
			int y2 = m_blockPixelAttributes.attribute(z - 1, x).height;
			int d = ((y - y1) + (y - y2)) * 12;
			if (d > 36) {
				d = 36;
			}
			int sourceColor = m_image->tpixels[imageY][getImageX(x)] & 0xffffff;
			int r = (sourceColor & 0xff0000) >> 16;
			int g = (sourceColor & 0x00ff00) >> 8;
			int b = (sourceColor & 0x0000ff);
			r = colorSafeBounds(r + d);
			g = colorSafeBounds(g + d);
			b = colorSafeBounds(b + d);
			m_image->tpixels[imageY][getImageX(x)] = rgb2int(r, g, b);
		}
	}
	m_blockPixelAttributes.scroll();
}

void TileGenerator::renderScale()
{
	int color = rgb2int(m_scaleColor.r, m_scaleColor.g, m_scaleColor.b);
	gdImageString(m_image, gdFontGetMediumBold(), 24, 0, reinterpret_cast<unsigned char *>(const_cast<char *>("X")), color);
	gdImageString(m_image, gdFontGetMediumBold(), 2, 24, reinterpret_cast<unsigned char *>(const_cast<char *>("Z")), color);

	string scaleText;

	for (int i = (m_xMin / 4) * 4; i <= m_xMax; i += 4) {
		stringstream buf;
		buf << i * 16;
		scaleText = buf.str();

		int xPos = getImageX(m_xMin * -16 + i * 16);
		gdImageString(m_image, gdFontGetMediumBold(), xPos + 2, 0, reinterpret_cast<unsigned char *>(const_cast<char *>(scaleText.c_str())), color);
		gdImageLine(m_image, xPos, 0, xPos, m_border - 1, color);
	}

	for (int i = (m_zMax / 4) * 4; i >= m_zMin; i -= 4) {
		stringstream buf;
		buf << i * 16;
		scaleText = buf.str();

		int yPos = getImageY(m_mapHeight - 1 - (i * 16 - m_zMin * 16));
		gdImageString(m_image, gdFontGetMediumBold(), 2, yPos, reinterpret_cast<unsigned char *>(const_cast<char *>(scaleText.c_str())), color);
		gdImageLine(m_image, 0, yPos, m_border - 1, yPos, color);
	}
}

void TileGenerator::renderOrigin()
{
	int imageX = getImageX(-m_xMin * 16);
	int imageY = getImageY(m_mapHeight - 1 - m_zMin * -16);
	gdImageArc(m_image, imageX, imageY, 12, 12, 0, 360, rgb2int(m_originColor.r, m_originColor.g, m_originColor.b));
}

void TileGenerator::renderPlayers(const std::string &inputPath)
{
	int color = rgb2int(m_playerColor.r, m_playerColor.g, m_playerColor.b);

	PlayerAttributes players(inputPath);
	for (PlayerAttributes::Players::iterator player = players.begin(); player != players.end(); ++player) {
		int imageX = getImageX(player->x / 10 - m_xMin * 16);
		int imageY = getImageY(m_mapHeight - 1 - (player->z / 10 - m_zMin * 16));

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

inline int TileGenerator::getImageX(int val) const
{
	if (m_tileWidth && m_tileBorderSize)
		val += ((val - m_tileMapXOffset + m_tileWidth) / m_tileWidth) * m_tileBorderSize + m_border;
	else
		val += m_border;
	return val;
}

inline int TileGenerator::getImageY(int val) const
{
	if (m_tileHeight && m_tileBorderSize)
		val += ((val - m_tileMapYOffset + m_tileHeight) / m_tileHeight) * m_tileBorderSize + m_border;
	else
		val += m_border;
	return val;
}

