/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  22.08.2012 15:15:54
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <map>
#include <string>
#include <cctype>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include "TileGenerator.h"
#include "PixelAttributes.h"

using namespace std;

#define OPT_SQLITE_CACHEWORLDROW	0x81
#define OPT_PROGRESS_INDICATOR		0x82
#define OPT_DRAW_OBJECT			0x83
#define OPT_BLOCKCOLOR			0x84
#define OPT_DRAWAIR			0x85
#define OPT_VERBOSE_SEARCH_COLORS	0x86
#define OPT_CHUNKSIZE			0x87
#define OPT_HEIGHTMAP			0x88
#define OPT_HEIGHTMAPYSCALE		0x89
#define OPT_HEIGHT_LEVEL0		0x8a
#define OPT_HEIGHTMAPNODESFILE		0x8b
#define OPT_HEIGHTMAPCOLORSFILE		0x8c
#define OPT_DRAWHEIGHTSCALE		0x8d
#define OPT_SCALEFACTOR			0x8e
#define OPT_SCALEINTERVAL		0x8f

// Will be replaced with the actual name and location of the executable (if found)
string executableName = "minetestmapper";
string installPrefix = INSTALL_PREFIX;
string nodeColorsDefaultFile = "colors.txt";
string heightMapNodesDefaultFile = "heightmap-nodes.txt";
string heightMapColorsDefaultFile = "heightmap-colors.txt";

class FuzzyBool {
private:
	int m_value;
	FuzzyBool(int i) : m_value(i) {}
public:
	FuzzyBool() : m_value(0) {}
	FuzzyBool(bool b) : m_value(b ? Yes.m_value : No.m_value) {}
	static const FuzzyBool Yes;
	static const FuzzyBool Maybe;
	static const FuzzyBool No;
inline friend bool operator==(FuzzyBool f1, FuzzyBool f2) { return f1.m_value == f2.m_value; }
inline friend bool operator!=(FuzzyBool f1, FuzzyBool f2) { return f1.m_value != f2.m_value; }
inline friend bool operator>=(FuzzyBool f1, FuzzyBool f2) { return f1.m_value >= f2.m_value; }
inline friend bool operator<=(FuzzyBool f1, FuzzyBool f2) { return f1.m_value <= f2.m_value; }
inline friend bool operator<(FuzzyBool f1, FuzzyBool f2) { return f1.m_value < f2.m_value; }
inline friend bool operator>(FuzzyBool f1, FuzzyBool f2) { return f1.m_value < f2.m_value; }
};
const FuzzyBool FuzzyBool::Yes = 1;
const FuzzyBool FuzzyBool::Maybe = 0;
const FuzzyBool FuzzyBool::No = -1;

void usage()
{
	const char *options_text = "[options]\n"
			"  -h/--help\n"
			"  -V/--version\n"
			"  -i/--input <world_path>\n"
			"  -o/--output <output_image.png>\n"
			"  --colors <file>\n"
			"  --heightmap-nodes <file>\n"
			"  --heightmap-colors[=<file>]\n"
			"  --height-level-0 <level>\n"
			"  --heightmap[=<color>]\n"
			"  --heightmap-yscale <scale>\n"
			"  --bgcolor <color>\n"
			"  --blockcolor <color>\n"
			"  --scalecolor <color>\n"
			"  --playercolor <color>\n"
			"  --origincolor <color>\n"
			"  --tilebordercolor <color>\n"
			"  --drawscale[=left,top]\n"
			"  --sidescale-interval <major>[[,:]<minor>]\n"
			"  --drawheightscale\n"
			"  --heightscale-interval <major>[[,:]<minor>]\n"
			"  --drawplayers\n"
			"  --draworigin\n"
			"  --drawalpha[=cumulative|cumulative-darken|average|none]\n"
			"  --drawair\n"
			"  --draw[map]point \"<x>,<y> color\"\n"
			"  --draw[map]line \"<geometry> color\"\n"
			"  --draw[map]circle \"<geometry> color\"\n"
			"  --draw[map]ellipse \"<geometry> color\"\n"
			"  --draw[map]rectangle \"<geometry> color\"\n"
			"  --draw[map]text \"<x>,<y> color text\"\n"
			"  --noshading\n"
			"  --min-y <y>\n"
			"  --max-y <y>\n"
			"  --backend <" USAGE_DATABASES ">\n"
			"  --geometry <geometry>\n"
			"\t(Warning: has a compatibility mode - see README.rst)\n"
			"  --cornergeometry <geometry>\n"
			"  --centergeometry <geometry>\n"
			"  --geometrymode pixel,block,fixed,shrink\n"
			"\tpixel:   interpret geometry as pixel-accurate\n"
			"\tblock:   round geometry away from zero, to entire map blocks (16 nodes)\n"
			"\tfixed:   generate a map of exactly the requested geometry\n"
			"\tshrink:  generate a smaller map if possible\n"
#if USE_SQLITE3
			"  --sqlite-cacheworldrow\n"
#endif
			"  --tiles <tilesize>[+<border>]|block|chunk\n"
			"  --tileorigin <x>,<y>|world|map\n"
			"  --tilecenter <x>,<y>|world|map\n"
			"  --scalefactor 1:<n>\n"
			"  --chunksize <size>\n"
			"  --verbose[=n]\n"
			"  --verbose-search-colors[=n]\n"
			"  --progress\n"
			"Color formats:\n"
			"\t'#000' or '#000000'                                  (RGB)\n"
			"\t'#0000' or '#0000000'                                (ARGB - usable if an alpha value is allowed)\n"
			"\tSome symbolic color names:\n"
			"\t\twhite, black, gray, grey, red, green, blue,\n"
			"\t\tyellow, magenta, fuchsia, cyan, aqua,\n"
			"\t\torange, chartreuse, pink, violet, springgreen, azure\n"
			"\t\tbrown (= 50% orange)\n"
			"\tAs well as <color>[+-][wkrgbcmy]<n>, where n = 0.0..1.0 (or 1.00001 .. 255)\n"
			"\t\t[+-][wkrgbcmy]<n> mixes in or out white, black, red, green, blue, cyan, magenta, yellow\n"
			"Geometry formats:\n"
			"\t<width>x<heigth>[+|-<xoffset>+|-<yoffset>]           (dimensions and corner)\n"
			"\t<xoffset>,<yoffset>+<width>+<height>                 (corner and dimensions)\n"
			"\t<xcenter>,<ycenter>:<width>x<height>                 (center and dimensions)\n"
			"\t<xcorner1>,<ycorner1>:<xcorner2>,<ycorner2>          (corners of area)\n"
			"\tOriginal/legacy format - see note under '--geometry' option:\n"
			"\t<xoffset>:<yoffset>+<width>+<height>                 (corner and dimensions)\n"
			"X and Y coordinate formats:\n"
			"\t[+-]<n>                                              (node +/- <n>)\n"
			"\t[+-]<b>#[<n>]                                        (node <n> in block +/- <b>)\n"
			"\t[+-]<b>.[<n>]                                        (node +/- (b * 16 + n))\n"
			;
	std::cout << executableName << ' ' << options_text;
}

void parseDataFile(TileGenerator &generator, const string &input, string dataFile, string defaultFile,
	void (TileGenerator::*parseFile)(const std::string &fileName))
{
	if (!dataFile.empty()) {
		(generator.*parseFile)(dataFile);
		return;
	}

	std::vector<std::string> colorPaths;
	colorPaths.push_back(input);

	// Check if input/../.. looks like a valid minetest directory
	string minetestPath = input + PATH_SEPARATOR + ".." + PATH_SEPARATOR + "..";
	string minetestConf = minetestPath + PATH_SEPARATOR + "minetest.conf";
	int fd;
	if (0 <= (fd = open(minetestConf.c_str(), O_RDONLY))) {
		close(fd);
		colorPaths.push_back(minetestPath);
	}
	char *homedir;
	if ((homedir = getenv("HOME"))) {
		colorPaths.push_back(string(homedir) + PATH_SEPARATOR + ".minetest");
	}

	if (!installPrefix.empty()) {
#if PACKAGING_FLAT
		colorPaths.push_back(installPrefix);
#else
		colorPaths.push_back(installPrefix + "/share/games/minetestmapper");
#endif
	}
	colorPaths.push_back("");

	std::vector<std::string> fileNames;
	fileNames.push_back(defaultFile);

	for (std::vector<std::string>::iterator path = colorPaths.begin(); path != colorPaths.end(); path++) {
		for (std::vector<std::string>::iterator name = fileNames.begin(); name != fileNames.end(); name++) {
			if (path->empty())
				dataFile = *name;
			else
				dataFile = *path + PATH_SEPARATOR +  *name;
			try {
				(generator.*parseFile)(dataFile);
				if (path->empty()) {
					// I hope this is not obnoxious to windows users ?
					cerr	<< "Warning: Using " << *name << " in current directory as a last resort." << std::endl
						<< "         Preferably, store the colors file in the world directory" << std::endl;
					if (homedir)
						cerr	<< "         or in the private minetest directory ($HOME/.minetest)." << std::endl;
					cerr	<< "         It can also be specified on the command-line" << std::endl;
				}
				return;
			} catch (std::runtime_error e) {
				// Ignore failure to locate colors file in standard location
				// (we have more places to search)
				if (path->empty()) {
				}
			}

		}
	}
	ostringstream oss;
	oss << "Failed to find or failed to open a " << defaultFile << " file.";
	throw std::runtime_error(oss.str().c_str());
}

// is: stream to read from
// coord: set to coordinate value that was read
// isBlockCoord: set to true if the coordinate read was a block coordinate
// wildcard: if non-zero, accept '*' as a coordinate, and return this value instead.
//	(suggested values for 'wildcard': INT_MIN or INT_MAX)
//
// Accepted coordinate syntax:
// 	[+-]<n>:	node coordinate:  node +/- n
// 	[+-]<b>#:	block coordinate: block +/- b	(isBlockCoord will be set to true)
// 	[+-]<b>#<n>:	node coordinate:  node <n> in block +/- <b>
// 	[+-]<b>.<n>:	node coordinate:  node +/- (b * 16 + n)
// As a special feature, double signs are also supported. E.g.:
//	+-3
// Which allows shell command-lines like the following
//	${width}x${height}+$xoffs+$yoffs
// (which otherwise require special measures to cope with xoffs or yoffs being negative...)
// Other uses of this feature are left as an excercise to the reader.
// Hint: --3.5 is *not* the same as 3.5
static bool parseNodeCoordinate(istream &is, int &coord, bool &isBlockCoord, int wildcard)
{
	char c;
	int i;
	char s;

	s = c = is.peek();
	if (c == '*') {
		if (wildcard) {
			i = wildcard;
			is.ignore(1);
		}
		else {
			is >> coord;	// Set stream status to failed
		}
	}
	else {
		wildcard = 0;		// not processing a wildcard now
		if (s == '-' || s == '+')
			is.ignore(1);
		else
			s = '+';
		is >> i;
		if (s == '-')
			i = -i;
	}
	if (is.fail())
		return false;
	coord = i;
	isBlockCoord = false;
	if (is.eof())
		return true;

	// Check if this is a block number, and so: if it has a node number.
	c = is.peek();
	if (c == '#' || c == '.') {
		// coordinate read was a block number
		is.ignore(1);
		if (wildcard) {
			return false;	// wildcards are generic
		}
		else if (isdigit(is.peek())) {
			// has a node number / offset
			is >> i;
			if (!is.fail()) {
				if (c == '.' && s == '-') {
					// Using '.', the node number has same sign as block number
					// Using '#', the node number is always positive
					// i.e. -1#1 is: node #1 in block -1 (i.e. node -16 + 1 = -15)
					// i.e. -1.1 is: 1 block and 1 node in negative direction (i.e. node 16 - 1 = -17)
					i = -i;
				}
				coord = coord * 16 + i;
			}
		}
		else {
			// No node number / offset
			isBlockCoord = true;
		}
	}
	return (!is.fail());
}

static bool parseCoordinates(istream &is, NodeCoord &coord, int n, int wildcard = 0, char separator = ',')
{
	bool result;
	result = true;
	NodeCoord tempCoord;
	for (int i = 0; result && i < n; i++) {
		if (i && separator) {
			char c;
			is >> c;
			if (c != separator) {
				result = false;
				break;
			}
		}
		result = parseNodeCoordinate(is, tempCoord.dimension[i], tempCoord.isBlock[i], wildcard);
	}
	if (result)
		coord = tempCoord;
	return result;
}

static void convertBlockToNodeCoordinates(NodeCoord &coord, int offset, int n)
{
	for (int i = 0; i < n; i++) {
		if (coord.isBlock[i]) {
			coord.dimension[i] = coord.dimension[i] * 16 + offset;
			coord.isBlock[i] = false;
		}
	}
}

static void convertBlockToNodeCoordinates(NodeCoord &coord1, NodeCoord &coord2, int n)
{
	for (int i = 0; i < n; i++) {
		int c1 = coord1.isBlock[i] ? coord1.dimension[i] * 16 : coord1.dimension[i];
		int c2 = coord2.isBlock[i] ? coord2.dimension[i] * 16 + 15 : coord2.dimension[i];
		if (c1 > c2) {
		    c1 = coord1.isBlock[i] ? coord1.dimension[i] * 16 + 15 : coord1.dimension[i];
		    c2 = coord2.isBlock[i] ? coord2.dimension[i] * 16 : coord2.dimension[i];
		}
		coord1.dimension[i] = c1;
		coord2.dimension[i] = c2;
		coord1.isBlock[i] = false;
		coord2.isBlock[i] = false;
	}
}

static void convertCenterToCornerCoordinates(NodeCoord &coord, NodeCoord &dimensions, int n)
{
	// This results in a slight bias to the negative side.
	// i.e.: 0,0:2x2 will be -1,-1 .. 0,0 and not 0,0 .. 1,1
	// The advantage is that e.g. 0#,0#:16x16 selects the 16x16 area that is block 0:
	// 	0#,0#:16x16 -> 0,0:15,15
	// With a bias to the positive side, that would be:
	// 	0#,0#:16x16 -> 1,1:16,16
	// Which is counter-intuitive by itself (IMHO :-)
	for (int i = 0; i < n; i++) {
		if (dimensions.dimension[i] < 0)
			coord.dimension[i] += -dimensions.dimension[i] / 2;
		else
			coord.dimension[i] -= dimensions.dimension[i] / 2;
	}
}

static void convertDimensionToCornerCoordinates(NodeCoord &coord1, NodeCoord &coord2, NodeCoord &dimensions, int n)
{
	for (int i = 0; i < n; i++) {
		if (dimensions.dimension[i] < 0)
			coord2.dimension[i] = coord1.dimension[i] + dimensions.dimension[i] + 1;
		else
			coord2.dimension[i] = coord1.dimension[i] + dimensions.dimension[i] - 1;
	}
}

static void orderCoordinateDimensions(NodeCoord &coord1, NodeCoord &coord2, int n)
{
	for (int i = 0; i < n; i++)
		if (coord1.dimension[i] > coord2.dimension[i]) {
			int temp = coord1.dimension[i];
			coord1.dimension[i] = coord2.dimension[i];
			coord2.dimension[i] = temp;
		}
}


// Parse the following geometry formats:
// <w>x<h>[+<x>+<y>]
//	(dimensions, and position)
//	(if x and y are omitted, they default to -w/2 and -h/2)
// <x1>,<y1>:<x2>,<y2>
//	(2 corners of the area)
// <x>,<y>:<w>x<h>
//	(center of the area, and dimensions)
// <x>[,:]<y>+<w>+<h>
//	(corner of the area, and dimensions)
static bool parseGeometry(istream &is, NodeCoord &coord1, NodeCoord &coord2, NodeCoord &dimensions, bool &legacy, bool &centered, int n, FuzzyBool expectDimensions, int wildcard = 0)
{
	int pos;
	pos = is.tellg();
	legacy = false;

	for (int i = 0; i < n; i++) {
		coord1.dimension[i] = NodeCoord::Invalid;
		coord2.dimension[i] = NodeCoord::Invalid;
		dimensions.dimension[i] = NodeCoord::Invalid;
	}

	if (expectDimensions >= FuzzyBool::Maybe && parseCoordinates(is, dimensions, n, 0, 'x')) {
		convertBlockToNodeCoordinates(dimensions, 0, n);
		// <w>x<h>[+<x>+<y>]
		if (is.eof()) {
			centered = true;
			for (int i = 0; i < n; i++) {
				coord1.dimension[i] = 0;
				coord1.isBlock[i] = false;
			}
			return (is.eof() || is.peek() == ' ' || is.peek() == '\t');
		}
		else {
			centered = false;
			if (parseCoordinates(is, coord1, n, 0, '\0')) {
				convertBlockToNodeCoordinates(coord1, 0, n);
				return (is.eof() || is.peek() == ' ' || is.peek() == '\t');
			}
			else
				return false;
		}
	}

	is.clear();
	is.seekg(pos);
	if (wildcard) {
		coord1.x = coord1.y = coord1.z = 0;
	}
	if (parseCoordinates(is, coord1, n, wildcard, ',')) {
		if (expectDimensions == FuzzyBool::No || (expectDimensions == FuzzyBool::Maybe && (is.eof() || is.peek() == ' ' || is.peek() == '\t'))) {
			// Just coordinates were specified
			centered = false;
			return (is.eof() || is.peek() == ' ' || is.peek() == '\t');
		}
		else if (wildcard && (coord1.x == wildcard || coord1.y == wildcard || coord1.z == wildcard)) {
			// wildcards are only allowed for plain coordinates (i.e. no dimensions)
			return false;
		}
		else if (is.peek() == ':') {
			is.ignore(1);
			pos = is.tellg();
			if (parseCoordinates(is, coord2, n, 0, ',')) {
				// <x1>,<y1>:<x2>,<y2>
				centered = false;
				convertBlockToNodeCoordinates(coord1, coord2, n);
				return (is.eof() || is.peek() == ' ' || is.peek() == '\t');
			}
			is.clear();
			is.seekg(pos);
			if (parseCoordinates(is, dimensions, n, 0, 'x')) {
				// <x>,<y>:<w>x<h>
				// (x,y is the center of the area by default)
				centered = true;
				convertBlockToNodeCoordinates(coord1, 8, n);
				convertBlockToNodeCoordinates(dimensions, 0, n);
				return (is.eof() || is.peek() == ' ' || is.peek() == '\t');
			}
			else {
				return false;
			}
		}
		else {
			// <x>,<y>+<w>+<h>
			centered = false;
			if (parseCoordinates(is, dimensions, n, 0, '\0')) {
				convertBlockToNodeCoordinates(coord1, 0, n);
				convertBlockToNodeCoordinates(dimensions, 0, n);
				return (is.eof() || is.peek() == ' ' || is.peek() == '\t');
			}
			else {
				return false;
			}
		}
	}

	is.clear();
	is.seekg(pos);
	if (parseCoordinates(is, coord1, n, 0, ':')) {
		// <x>:<y>+<w>+<h>
		legacy = true;
		centered = false;
		if (parseCoordinates(is, dimensions, n, 0, '\0')) {
			convertBlockToNodeCoordinates(coord1, 0, n);
			convertBlockToNodeCoordinates(dimensions, 0, n);
			return (is.eof() || is.peek() == ' ' || is.peek() == '\t');
		}
		return false;
	}

	return false;
}

static bool parseMapGeometry(istream &is, NodeCoord &coord1, NodeCoord &coord2, bool &legacy, FuzzyBool interpretAsCenter)
{
	NodeCoord dimensions;
	bool centered;

	bool result = parseGeometry(is, coord1, coord2, dimensions, legacy, centered, 2, FuzzyBool::Yes);

	if (result) {
		bool haveCoord2 = coord2.dimension[0] != NodeCoord::Invalid
			&& coord2.dimension[1] != NodeCoord::Invalid;
		bool haveDimensions = dimensions.dimension[0] != NodeCoord::Invalid
			&& dimensions.dimension[1] != NodeCoord::Invalid;
		if (!haveCoord2 && haveDimensions) {
			// Convert coord1 + dimensions to coord1 + coord2.
			// First, if coord1 must be interpreted as center of the area, adjust it to be a corner
			if ((centered && interpretAsCenter == FuzzyBool::Maybe) || interpretAsCenter == FuzzyBool::Yes)
				convertCenterToCornerCoordinates(coord1, dimensions, 2);
			convertDimensionToCornerCoordinates(coord1, coord2, dimensions, 2);
		}
		else if (!haveCoord2 || haveDimensions) {
			return false;
		}
		orderCoordinateDimensions(coord1, coord2, 2);
	}

	return result;
}

int main(int argc, char *argv[])
{
	if (argc) {
		string argv0 = argv[0];
		size_t pos = argv0.find_last_of(PATH_SEPARATOR);
		if (pos == string::npos) {
			if (!argv0.empty())
				executableName = argv0;
		}
		else {
			executableName = argv0.substr(pos + 1);
		}
	}

	static struct option long_options[] =
	{
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'V'},
		{"input", required_argument, 0, 'i'},
		{"output", required_argument, 0, 'o'},
		{"colors", required_argument, 0, 'C'},
		{"heightmap-nodes", required_argument, 0, OPT_HEIGHTMAPNODESFILE},
		{"heightmap-colors", required_argument, 0, OPT_HEIGHTMAPCOLORSFILE},
		{"heightmap", optional_argument, 0, OPT_HEIGHTMAP},
		{"heightmap-yscale", required_argument, 0, OPT_HEIGHTMAPYSCALE},
		{"height-level-0", required_argument, 0, OPT_HEIGHT_LEVEL0},
		{"bgcolor", required_argument, 0, 'b'},
		{"blockcolor", required_argument, 0, OPT_BLOCKCOLOR},
		{"scalecolor", required_argument, 0, 's'},
		{"origincolor", required_argument, 0, 'r'},
		{"playercolor", required_argument, 0, 'p'},
		{"draworigin", no_argument, 0, 'R'},
		{"drawplayers", no_argument, 0, 'P'},
		{"drawscale", optional_argument, 0, 'S'},
		{"sidescale-interval", required_argument, 0, OPT_SCALEINTERVAL},
		{"drawheightscale", no_argument, 0, OPT_DRAWHEIGHTSCALE},
		{"heightscale-interval", required_argument, 0, OPT_SCALEINTERVAL},
		{"drawalpha", optional_argument, 0, 'e'},
		{"drawair", no_argument, 0, OPT_DRAWAIR},
		{"drawpoint", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawline", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawcircle", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawellipse", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawrectangle", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawtext", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawmappoint", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawmapline", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawmapcircle", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawmapellipse", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawmaprectangle", required_argument, 0, OPT_DRAW_OBJECT},
		{"drawmaptext", required_argument, 0, OPT_DRAW_OBJECT},
		{"noshading", no_argument, 0, 'H'},
		{"geometry", required_argument, 0, 'g'},
		{"cornergeometry", required_argument, 0, 'g'},
		{"centergeometry", required_argument, 0, 'g'},
		{"geometrymode", required_argument, 0, 'G'},
		{"forcegeometry", no_argument, 0, 'G'},
		{"min-y", required_argument, 0, 'a'},
		{"max-y", required_argument, 0, 'c'},
		{"backend", required_argument, 0, 'd'},
		{"sqlite-cacheworldrow", no_argument, 0, OPT_SQLITE_CACHEWORLDROW},
		{"tiles", required_argument, 0, 't'},
		{"tileorigin", required_argument, 0, 'T'},
		{"tilecenter", required_argument, 0, 'T'},
		{"tilebordercolor", required_argument, 0, 'B'},
		{"scalefactor", required_argument, 0, OPT_SCALEFACTOR},
		{"chunksize", required_argument, 0, OPT_CHUNKSIZE},
		{"verbose", optional_argument, 0, 'v'},
		{"verbose-search-colors", optional_argument, 0, OPT_VERBOSE_SEARCH_COLORS},
		{"progress", no_argument, 0, OPT_PROGRESS_INDICATOR},
		{NULL, 0, 0, 0}
	};

	string input;
	string output;
	bool heightMap = false;
	bool loadHeightMapColorsFile = false;
	string nodeColorsFile;
	string heightMapColorsFile;
	string heightMapNodesFile;
	bool foundGeometrySpec = false;
	bool setFixedOrShrinkGeometry = false;

	TileGenerator generator;
	try {
		int option_index = 0;
		int c = 0;
		while (1) {
			c = getopt_long(argc, argv, "hi:o:", long_options, &option_index);
			if (c == -1) {
				if (input.empty() || output.empty()) {
					std::cerr << "Input (world directory) or output (PNG filename) missing" << std::endl;
					usage();
					return 0;
				}
				break;
			}
			switch (c) {
				case 'h':
					usage();
					return 0;
					break;
				case 'V':
					cout << "Minetestmapper - Version-ID: " << VERSION_MAJOR << "." << VERSION_MINOR << std::endl;
					return 0;
					break;
				case 'i':
					input = optarg;
					break;
				case 'o':
					output = optarg;
					break;
				case 'C':
					nodeColorsFile = optarg;
					break;
				case OPT_HEIGHTMAPNODESFILE:
					heightMapNodesFile = optarg;
					break;
				case OPT_HEIGHTMAPCOLORSFILE:
					heightMapColorsFile = optarg;
					break;
				case 'b':
					generator.setBgColor(Color(optarg, 0));
					break;
				case OPT_HEIGHTMAP:
					generator.setHeightMap(true);
					heightMap = true;
					if (optarg && *optarg) {
						loadHeightMapColorsFile = false;
						std::string color = optarg;
						int l = color.length();
						for (int i = 0; i < l; i++)
							color[i] = tolower(color[i]);
						if (color == "grey" || color == "gray")
							generator.setHeightMapColor(Color(0, 0, 0), Color(255, 255, 255));
						else if (color == "black")
							generator.setHeightMapColor(Color(0, 0, 0), Color(0, 0, 0));
						else if (color == "white")
							generator.setHeightMapColor(Color(255, 255, 255), Color(255, 255, 255));
						else
							generator.setHeightMapColor(Color(0, 0, 0), Color(color, 0));
						break;
					}
					else {
						loadHeightMapColorsFile = true;
					}
					break;
				case OPT_HEIGHTMAPYSCALE:
					if (isdigit(optarg[0]) || ((optarg[0]=='-' || optarg[0]=='+') && isdigit(optarg[1]))) {
						float scale = atof(optarg);
						generator.setHeightMapYScale(scale);
					}
					else {
						std::cerr << "Invalid parameter to '" << long_options[option_index].name << "': '" << optarg << "'" << std::endl;
						usage();
						exit(1);
					}
					break;
				case OPT_HEIGHT_LEVEL0:
					if (isdigit(optarg[0]) || ((optarg[0]=='-' || optarg[0]=='+') && isdigit(optarg[1]))) {
						int level = atoi(optarg);
						generator.setSeaLevel(level);
					}
					else {
						std::cerr << "Invalid parameter to '" << long_options[option_index].name << "': '" << optarg << "'" << std::endl;
						usage();
						exit(1);
					}
					break;
				case OPT_BLOCKCOLOR:
					generator.setBlockDefaultColor(Color(optarg, 0));
					break;
				case 's':
					generator.setScaleColor(Color(optarg,0));
					break;
				case 'r':
					generator.setOriginColor(Color(optarg,1));
					break;
				case 'p':
					generator.setPlayerColor(Color(optarg,1));
					break;
				case 'B':
					generator.setTileBorderColor(Color(optarg,0));
					break;
				case 'R':
					generator.setDrawOrigin(true);
					break;
				case 'P':
					generator.setDrawPlayers(true);
					break;
				case 'S':
					if (optarg && *optarg) {
						if (0 == strcasecmp(optarg,"left"))
							generator.setDrawScale(DRAWSCALE_LEFT);
						else if (0 == strcasecmp(optarg,"top"))
							generator.setDrawScale(DRAWSCALE_TOP);
						else if (0 == strcasecmp(optarg,"left,top"))
							generator.setDrawScale(DRAWSCALE_LEFT | DRAWSCALE_TOP);
						else if (0 == strcasecmp(optarg,"top,left"))
							generator.setDrawScale(DRAWSCALE_LEFT | DRAWSCALE_TOP);
						else {
							std::cerr << "Invalid parameter to '" << long_options[option_index].name
								<< "': '" << optarg << "' (expected: left,top)" << std::endl;
							usage();
							exit(1);
						}
					}
					else {
						generator.setDrawScale(DRAWSCALE_LEFT | DRAWSCALE_TOP);
					}
					break;
				case OPT_DRAWHEIGHTSCALE :
					generator.setDrawHeightScale(DRAWHEIGHTSCALE_BOTTOM);
					break;
				case OPT_SCALEINTERVAL: {
						istringstream arg;
						arg.str(optarg);
						int major;
						int minor;
						char sep;
						arg >> major;
						if (major < 0 || !isdigit(*optarg) || arg.fail()) {
							std::cerr << "Invalid parameter to '" << long_options[option_index].name
								<< "': '" << optarg << "' (expected: <major>[,<minor>]" << std::endl;
							usage();
							exit(1);
						}
						arg >> std::ws >> sep >> std::ws;
						if (!arg.fail()) {
							if ((sep != ',' && sep != ':') || !isdigit(arg.peek())) {
								std::cerr << "Invalid parameter to '" << long_options[option_index].name
									<< "': '" << optarg << "' (expected: <major>[,<minor>]" << std::endl;
								usage();
								exit(1);
							}
							arg >> minor;
							if (minor < 0) {
								std::cerr << "Invalid parameter to '" << long_options[option_index].name
									<< "': '" << optarg << "' (expected: <major>[,<minor>]" << std::endl;
								usage();
								exit(1);
							}
						}
						else {
							minor = 0;
						}
						if (minor && sep == ':') {
							if (major % minor) {
								std::cerr << long_options[option_index].name << ": Cannot divide major interval in "
									<< minor << " subintervals (not divisible)" << std::endl;
								exit(1);
							}
							minor = major / minor;
						}
						if ((minor % major) == 0)
							minor = 0;
						if (long_options[option_index].name[0] == 's') {
							generator.setSideScaleInterval(major, minor);
						}
						else if (long_options[option_index].name[0] == 'h') {
							generator.setHeightScaleInterval(major, minor);
						}
						else {
							std::cerr << "Internal error: option " << long_options[option_index].name << " not handled" << std::endl;
							exit(1);
						}
					}
					break;
				case 'v':
					if (optarg && isdigit(optarg[0]) && optarg[1] == '\0') {
						if (optarg[0] == '0')
							generator.verboseStatistics = false;
						else
							generator.verboseStatistics = true;
						generator.verboseCoordinates = optarg[0] - '0';
					}
					else {
						generator.verboseStatistics = true;
						generator.verboseCoordinates = 1;
					}
					break;
				case OPT_VERBOSE_SEARCH_COLORS:
					if (optarg && isdigit(optarg[0]) && optarg[1] == '\0') {
						generator.verboseReadColors = optarg[0] - '0';
					}
					else {
						generator.verboseReadColors++;
					}
					break;
				case 'e':
					generator.setDrawAlpha(true);
					if (!optarg || !*optarg)
						PixelAttribute::setMixMode(PixelAttribute::AlphaMixAverage);
					else if (string(optarg) == "cumulative" || string(optarg) == "nodarken")
						// "nodarken" is supported for backwards compatibility
						PixelAttribute::setMixMode(PixelAttribute::AlphaMixCumulative);
					else if (string(optarg) == "darken" || string(optarg) == "cumulative-darken")
						// "darken" is supported for backwards compatibility
						PixelAttribute::setMixMode(PixelAttribute::AlphaMixCumulativeDarken);
					else if (string(optarg) == "average")
						PixelAttribute::setMixMode(PixelAttribute::AlphaMixAverage);
					else if (string(optarg) == "none")
						generator.setDrawAlpha(false);
					else {
						std::cerr << "Invalid parameter to '" << long_options[option_index].name << "': '" << optarg << "'" << std::endl;
						usage();
						exit(1);
					}
					break;
				case OPT_DRAWAIR:
					generator.setDrawAir(true);
					break;
				case 'H':
					generator.setShading(false);
					break;
				case OPT_SQLITE_CACHEWORLDROW:
					generator.setSqliteCacheWorldRow(true);
					break;
				case OPT_PROGRESS_INDICATOR:
					generator.enableProgressIndicator();
					break;
				case 'a': {
						istringstream iss;
						iss.str(optarg);
						int miny;
						iss >> miny;
						generator.setMinY(miny);
					}
					break;
				case 'c': {
						istringstream iss;
						iss.str(optarg);
						int maxy;
						iss >> maxy;
						generator.setMaxY(maxy);
					}
					break;
				case OPT_CHUNKSIZE : {
						istringstream iss;
						iss.str(optarg);
						int size;
						iss >> size;
						if (iss.fail() || size < 0) {
							std::cerr << "Invalid chunk size (" << optarg << ")" << std::endl;
							usage();
							exit(1);
						}
						generator.setChunkSize(size);
					}
					break;
				case OPT_SCALEFACTOR: {
						istringstream arg;
						arg.str(optarg);
						int one;
						char colon;
						int factor = 1;
						arg >> one >> std::ws;
						if (arg.fail() || one != 1) {
							std::cerr << "Invalid scale factor specification (" << optarg << ") - expected: 1:<n>" << std::endl;
							exit(1);
						}
						if (!arg.eof()) {
							arg >> colon >> factor >> std::ws;
							if (arg.fail() || colon != ':' || factor<0 || !arg.eof()) {
								std::cerr << "Invalid scale factor specification (" << optarg << ") - expected: 1:<n>" << std::endl;
								usage();
								exit(1);
							}
							if (factor != 1 && factor != 2 && factor != 4 && factor != 8 && factor != 16) {
								std::cerr << "Scale factor must be 1:1, 1:2, 1:4, 1:8 or 1:16" << std::endl;
								exit(1);
							}
						}
						generator.setScaleFactor(factor);
					}
					break;
				case 't': {
						istringstream tilesize;
						tilesize.str(optarg);
						if (tilesize.str() == "block") {
							generator.setTileSize(BLOCK_SIZE, BLOCK_SIZE);
							generator.setTileOrigin(TILECORNER_AT_WORLDCENTER, TILECORNER_AT_WORLDCENTER);
							}
						else if (tilesize.str() == "chunk") {
							generator.setTileSize(TILESIZE_CHUNK, TILESIZE_CHUNK);
							generator.setTileOrigin(TILECENTER_AT_CHUNKCENTER, TILECENTER_AT_CHUNKCENTER);
							}
						else {
							int size, border;
							char c;
							tilesize >> size;
							if (tilesize.fail() || size<0) {
								std::cerr << "Invalid tile size specification (" << optarg << ")" << std::endl;
								usage();
								exit(1);
							}
							generator.setTileSize(size, size);
							tilesize >> c >> border;
							if (!tilesize.fail()) {
								if (c != '+' || border < 1) {
									std::cerr << "Invalid tile border size specification (" << optarg << ")" << std::endl;
									usage();
									exit(1);
								}
								generator.setTileBorderSize(border);
							}
						}
					}
					break;
				case 'T': {
						bool origin = long_options[option_index].name[4] == 'o';
						istringstream iss;
						iss.str(optarg);
						NodeCoord coord;
						if (iss.str() == "world") {
							if (origin)
								generator.setTileOrigin(TILECORNER_AT_WORLDCENTER, TILECORNER_AT_WORLDCENTER);
							else
								generator.setTileCenter(TILECENTER_AT_WORLDCENTER, TILECENTER_AT_WORLDCENTER);
						}
						else if (iss.str() == "map") {
							if (origin)
								generator.setTileOrigin(TILECORNER_AT_MAPCENTER, TILECORNER_AT_MAPCENTER);
							else
								generator.setTileCenter(TILECENTER_AT_MAPCENTER, TILECENTER_AT_MAPCENTER);
						}
						else {
							bool result = true;
							if (!parseCoordinates(iss, coord, 2, 0, ',')) {
								iss.str(optarg);
								result = parseCoordinates(iss, coord, 2, 0, ':');
							}
							if (result) {
								if (origin) {
									convertBlockToNodeCoordinates(coord, 0, 2);
									generator.setTileOrigin(coord.x, coord.y);
								}
								else {
									convertBlockToNodeCoordinates(coord, 8, 2);
									generator.setTileCenter(coord.x, coord.y);
								}
							}
							else {
								std::cerr << "Invalid " << long_options[option_index].name << " parameter (" << optarg << ")" << std::endl;
								usage();
								exit(1);
							}
						}
					}
					break;
				case 'G':
					if (long_options[option_index].name[0] == 'f') {
						// '--forcegeometry'
						// Old behavior - for compatibility.
						generator.setShrinkGeometry(false);
						setFixedOrShrinkGeometry = true;
						if (!foundGeometrySpec)
							generator.setBlockGeometry(true);
					}
					else if (optarg && *optarg) {
						for (char *c = optarg; *c; c++)
							if (*c == ',') *c = ' ';
						istringstream iss;
						iss.str(optarg);
						iss >> std::skipws;
						string flag;
						while (!iss.eof() && !iss.fail()) {
							iss >> flag;
							if (flag == "")
								(void) true;	// Empty flag - ignore
							else if (flag == "pixel")
								generator.setBlockGeometry(false);
							else if (flag == "block")
								generator.setBlockGeometry(true);
							else if (flag == "fixed")
								generator.setShrinkGeometry(false);
							else if (flag == "shrink")
								generator.setShrinkGeometry(true);
							else {
								std::cerr << "Invalid geometry mode flag '" << flag << "'" << std::endl;
								usage();
								exit(1);
							}
							if (flag == "fixed" || flag == "shrink")
								setFixedOrShrinkGeometry = true;
						}
						if (iss.fail()) {
							// Don't know when / if this could happen...
							std::cerr << "Error parsing geometry mode flags" << std::endl;
							usage();
							exit(1);
						}
					}
					foundGeometrySpec = true;
					break;
				case 'g': {
						istringstream iss;
						iss.str(optarg);
						NodeCoord coord1;
						NodeCoord coord2;
						bool legacy;
						FuzzyBool center = FuzzyBool::Maybe;
						if (long_options[option_index].name[0] == 'c' && long_options[option_index].name[1] == 'e')
							center = FuzzyBool::Yes;
						if (long_options[option_index].name[0] == 'c' && long_options[option_index].name[1] == 'o')
							center = FuzzyBool::No;
						if (!parseMapGeometry(iss, coord1, coord2, legacy, center)) {
							std::cerr << "Invalid geometry specification '" << optarg << "'" << std::endl;
							usage();
							exit(1);
						}
						// Set defaults
						if (!foundGeometrySpec) {
							if (long_options[option_index].name[0] == 'g' && legacy) {
								// Compatibility when using the option 'geometry'
								generator.setBlockGeometry(true);
								generator.setShrinkGeometry(true);
							}
							else {
								generator.setBlockGeometry(false);
								generator.setShrinkGeometry(false);
							}
							setFixedOrShrinkGeometry = true;
						}
						if (!setFixedOrShrinkGeometry) {
							// Special treatement is needed, because:
							// - without any -[...]geometry option, default is shrink
							// - with    any -[...]geometry option, default is fixed
							generator.setShrinkGeometry(false);
							setFixedOrShrinkGeometry = true;
						}
						generator.setGeometry(coord1, coord2);
						foundGeometrySpec = true;
					}
					break;
				case OPT_DRAW_OBJECT: {
						TileGenerator::DrawObject drawObject;
						drawObject.world = long_options[option_index].name[4] != 'm';
						char object = long_options[option_index].name[4 + (drawObject.world ? 0 : 3)];
						switch (object) {
						case 'p' :
							drawObject.type = TileGenerator::DrawObject::Point;
							break;
						case 'l' :
							drawObject.type = TileGenerator::DrawObject::Line;
							break;
						case 'r' :
							drawObject.type = TileGenerator::DrawObject::Rectangle;
							break;
						case 'e' :
						case 'c' :
							drawObject.type = TileGenerator::DrawObject::Ellipse;
							break;
						case 't' :
							drawObject.type = TileGenerator::DrawObject::Text;
							break;
						default :
							std::cerr << "Internal error: unrecognised object ("
								<< long_options[option_index].name
								<< ")" << std::endl;
							exit(1);
							break;
						}

						istringstream iss;
						iss.str(optarg);
						NodeCoord coord1;
						NodeCoord coord2;
						NodeCoord dimensions;
						FuzzyBool needDimensions;
						bool legacy;
						bool centered;

						if (object == 'p' || object == 't')
							needDimensions = FuzzyBool::No;
						else
							needDimensions = FuzzyBool::Yes;
						if (!parseGeometry(iss, coord1, coord2, dimensions, legacy, centered, 2, needDimensions)) {
							std::cerr << "Invalid drawing geometry specification for "
								<< long_options[option_index].name
								<< " '" << optarg << "'" << std::endl;
							usage();
							exit(1);
						}
						bool haveCoord2 = coord2.dimension[0] != NodeCoord::Invalid
							&& coord2.dimension[1] != NodeCoord::Invalid;
						bool haveDimensions = dimensions.dimension[0] != NodeCoord::Invalid
							&& dimensions.dimension[1] != NodeCoord::Invalid;

						if (object == 'p' || object == 't') {
							for (int i = 0; i < 2; i++)
								if (coord1.isBlock[i]) {
									coord1.dimension[i] *= 16;
									coord1.isBlock[i] = false;
								}
							drawObject.setCenter(coord1);
							drawObject.setDimensions(NodeCoord(1,1,1));
						}
						else {
							if (haveDimensions) {
								if (centered)
									drawObject.setCenter(coord1);
								else
									drawObject.setCorner1(coord1);
								drawObject.setDimensions(dimensions);
							}
							else if (haveCoord2) {
								drawObject.setCorner1(coord1);
								drawObject.setCorner2(coord2);
							}
							else {
#ifdef DEBUG
								assert(!haveDimensions && !haveCoord2);
#else
								break;
#endif
							}
						}

						string colorStr;
						iss >> std::ws >> colorStr;
						if (iss.fail()) {
							std::cerr << "Invalid color specification for "
								<< long_options[option_index].name
								<< " '" << optarg << "'" << std::endl;
							usage();
							exit(1);
						}
						drawObject.color = colorStr;

						if (object == 't') {
							iss >> std::ws;
							std::getline(iss, drawObject.text);
							if (drawObject.text.empty() || iss.fail()) {
								std::cerr << "Invalid or missing text for "
									<< long_options[option_index].name
									<< " '" << optarg << "'" << std::endl;
								usage();
								exit(1);
							}
						}

						generator.drawObject(drawObject);
					}
					break;
				case 'd':
					generator.setBackend(optarg);
					break;
				default:
					exit(1);
			}
		}
	} catch(std::runtime_error e) {
		std::cout<<"Command-line error: "<<e.what()<<std::endl;
		return 1;
	}

	try {
		if (heightMap) {
			parseDataFile(generator, input, heightMapNodesFile, heightMapNodesDefaultFile, &TileGenerator::parseHeightMapNodesFile);
			if (loadHeightMapColorsFile)
				parseDataFile(generator, input, heightMapColorsFile, heightMapColorsDefaultFile, &TileGenerator::parseHeightMapColorsFile);
		}
		else {
			parseDataFile(generator, input, nodeColorsFile, nodeColorsDefaultFile, &TileGenerator::parseNodeColorsFile);
		}
		generator.generate(input, output);
	} catch(std::runtime_error e) {
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return 1;
	}
	return 0;
}

