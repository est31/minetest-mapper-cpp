/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  23.08.2012 12:35:59
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#ifndef TILEGENERATOR_H_JJNUCARH
#define TILEGENERATOR_H_JJNUCARH

#include <gd.h>
#include <climits>
#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include "PixelAttributes.h"
#include "Color.h"
#include "db.h"

#define MINETEST_MAPBLOCK_MIN	(-2048)
#define MINETEST_MAPBLOCK_MAX	2047

#define TILECENTER_IS_WORLDCENTER	INT_MAX
#define TILECENTER_IS_MAPCENTER		INT_MIN

struct BlockPos {
	int x;
	int y;
	int z;
	// operator< should order the positions in the
	// order the corresponding pixels are generated:
	// First (most significant): z coordinate, descending (i.e. reversed)
	// Then                    : x coordinate, ascending
	// Last (least significant): y coordinate, descending (i.e. reversed)
	bool operator<(const BlockPos& p) const
	{
		if (z > p.z) {
			return true;
		}
		if (z < p.z) {
			return false;
		}
		if (x < p.x) {
			return true;
		}
		if (x > p.x) {
			return false;
		}
		if (y > p.y) {
			return true;
		}
		if (y < p.y) {
			return false;
		}
		return false;
	}
	bool operator==(const BlockPos& p) const
	{
		if (z != p.z) {
			return false;
		}
		if (y != p.y) {
			return false;
		}
		if (x != p.x) {
			return false;
		}
		return true;
	}
};


class TileGenerator
{
private:
	typedef std::basic_string<unsigned char> unsigned_string;
	typedef std::map<std::string, ColorEntry> ColorMap;
	typedef std::pair<BlockPos, unsigned_string> Block;
	typedef std::list<Block> BlockList;

public:
	TileGenerator();
	~TileGenerator();
	void setBgColor(const std::string &bgColor);
	void setScaleColor(const std::string &scaleColor);
	void setOriginColor(const std::string &originColor);
	void setPlayerColor(const std::string &playerColor);
	Color parseColor(const std::string &color);
	void setDrawOrigin(bool drawOrigin);
	void setDrawPlayers(bool drawPlayers);
	void setDrawScale(bool drawScale);
	void setDrawAlpha(bool drawAlpha);
	void setShading(bool shading);
	void setGeometry(int x, int y, int w, int h);
	void setMinY(int y);
	void setMaxY(int y);
	void setShrinkGeometry(bool shrink);
	void setBlockGeometry(bool block);
	void setSqliteCacheWorldRow(bool cacheWorldRow);
	void setTileBorderColor(const std::string &tileBorderColor);
	void setTileBorderSize(int size);
	void setTileSize(int width, int heigth);
	void setTileOrigin(int x, int y);
	void enableProgressIndicator(void);
	void parseColorsFile(const std::string &fileName);
	void setBackend(std::string backend);
	void generate(const std::string &input, const std::string &output);

private:
	void parseColorsStream(std::istream &in, const std::string &filename);
	void openDb(const std::string &input);
	void loadBlocks();
	BlockPos decodeBlockPos(int64_t blockId) const;
	void createImage();
	void computeMapParameters();
	void renderMap();
	std::list<int> getZValueList() const;
	Block getBlockOnPos(BlockPos pos);
	void pushPixelRows(int zPosLimit);
	void renderMapBlock(const unsigned_string &mapBlock, const BlockPos &pos, int version);
	void renderScale();
	void renderOrigin();
	void renderPlayers(const std::string &inputPath);
	void writeImage(const std::string &output);
	void printUnknown();
	int mapX2ImageX(int val) const;
	int mapY2ImageY(int val) const;
	int worldX2ImageX(int val) const;
	int worldZ2ImageY(int val) const;
	int worldBlockX2StoredX(int xPos) const { return (xPos - m_xMin) * 16; }
	int worldBlockZ2StoredY(int zPos) const { return (m_zMax - zPos) * 16; }

public:
	bool verboseCoordinates;
	bool verboseStatistics;
	bool progressIndicator;

private:
	Color m_bgColor;
	Color m_scaleColor;
	Color m_originColor;
	Color m_playerColor;
	Color m_tileBorderColor;
	bool m_drawOrigin;
	bool m_drawPlayers;
	bool m_drawScale;
	bool m_drawAlpha;
	bool m_shading;
	int m_border;
	std::string m_backend;
	bool m_shrinkGeometry;
	bool m_blockGeometry;
	bool m_sqliteCacheWorldRow;

	DB *m_db;
	gdImagePtr m_image;
	PixelAttributes m_blockPixelAttributes;
	int m_xMin;
	int m_xMax;
	int m_zMin;
	int m_zMax;
	int m_yMin;
	int m_yMax;
	int m_reqXMin;
	int m_reqXMax;
	int m_reqYMin;
	int m_reqYMax;
	int m_reqZMin;
	int m_reqZMax;
	int m_reqYMinNode;		// Node offset within a map block
	int m_reqYMaxNode;		// Node offset within a map block
	int m_storedWidth;
	int m_storedHeight;
	int m_mapXStartNodeOffset;
	int m_mapYStartNodeOffset;
	int m_mapXEndNodeOffset;
	int m_mapYEndNodeOffset;
	int m_nextStoredYCoord;
	int m_tileXOrigin;
	int m_tileZOrigin;
	int m_tileWidth;
	int m_tileHeight;
	int m_tileBorderSize;
	int m_tileMapXOffset;
	int m_tileMapYOffset;
	int m_tileBorderXCount;
	int m_tileBorderYCount;
	int m_pictWidth;
	int m_pictHeight;
	std::list<BlockPos> m_positions;
	std::map<int, std::string> m_nameMap;
	ColorMap m_colors;
	uint16_t m_readedPixels[16];
	std::set<std::string> m_unknownNodes;

	int m_blockAirId;
	int m_blockIgnoreId;
}; /* -----  end of class TileGenerator  ----- */

#endif /* end of include guard: TILEGENERATOR_H_JJNUCARH */

