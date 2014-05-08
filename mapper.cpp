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
#include <sstream>
#include <stdexcept>
#include "TileGenerator.h"
#include "ZlibDecompressor.h"

using namespace std;

#define OPT_SQLITE_CACHEWORLDROW	0x81
#define OPT_PROGRESS_INDICATOR		0x82

void usage()
{
	const char *usage_text = "minetestmapper [options]\n"
			"  -h/--help\n"
			"  -i/--input <world_path>\n"
			"  -o/--output <output_image.png>\n"
			"  --colors <file>\n"
			"  --bgcolor <color>\n"
			"  --scalecolor <color>\n"
			"  --playercolor <color>\n"
			"  --origincolor <color>\n"
			"  --tilebordercolor <color>\n"
			"  --drawscale\n"
			"  --drawplayers\n"
			"  --draworigin\n"
			"  --drawalpha\n"
			"  --noshading\n"
			"  --min-y <y>\n"
			"  --max-y <y>\n"
			"  --backend <sqlite3/leveldb>\n"
			"  --geometry <geometry>\n"
			"  --cornergeometry <geometry>\n"
			"  --centergeometry <geometry>\n"
			"  --geometrymode pixel,block,fixed,shrink\n"
			"  --sqlite-cacheworldrow\n"
			"  --tiles <tilesize>[+<border>]\n"
			"  --tileorigin <x>,<y>|center-world|center-map\n"
			"  --verbose[=n]\n"
			"  --progress\n"
			"Color format: '#000' or '#000000'\n"
			"Geometry formats:\n"
			"\t<width>x<heigth>[+|-<xoffset>+|-<yoffset>]\n"
			"\t<xoffset>:<yoffset>+<width>+<height>\n";
	std::cout << usage_text;
}

int main(int argc, char *argv[])
{
	static struct option long_options[] =
	{
		{"help", no_argument, 0, 'h'},
		{"input", required_argument, 0, 'i'},
		{"output", required_argument, 0, 'o'},
		{"colors", required_argument, 0, 'C'},
		{"bgcolor", required_argument, 0, 'b'},
		{"scalecolor", required_argument, 0, 's'},
		{"origincolor", required_argument, 0, 'r'},
		{"playercolor", required_argument, 0, 'p'},
		{"draworigin", no_argument, 0, 'R'},
		{"drawplayers", no_argument, 0, 'P'},
		{"drawscale", no_argument, 0, 'S'},
		{"drawalpha", no_argument, 0, 'e'},
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
		{"tilebordercolor", required_argument, 0, 'B'},
		{"verbose", optional_argument, 0, 'v'},
		{"progress", no_argument, 0, OPT_PROGRESS_INDICATOR},
	};

	string input;
	string output;
	string colorsFile;
	bool foundGeometrySpec = false;

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
				case 'i':
					input = optarg;
					break;
				case 'o':
					output = optarg;
					break;
				case 'C':
					colorsFile = optarg;
					break;
				case 'b':
					generator.setBgColor(Color(optarg, 0));
					break;
				case 's':
					generator.setScaleColor(Color(optarg,0));
					break;
				case 'r':
					generator.setOriginColor(Color(optarg,0));
					break;
				case 'p':
					generator.setPlayerColor(Color(optarg,0));
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
					generator.setDrawScale(true);
					break;
				case 'v':
					generator.verboseStatistics = true;
					generator.verboseCoordinates = 1;
					if (optarg) {
						if (optarg[0] >= '2')
							generator.verboseCoordinates = 2;
					}
					break;
				case 'e':
					generator.setDrawAlpha(true);
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
				case 't': {
						istringstream tilesize;
						tilesize.str(optarg);
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
					break;
				case 'T': {
						istringstream origin;
						origin.str(optarg);
						int x, y;
						char c;
						origin >> x >> c >> y;
						if (origin.fail() || (c != ':' && c != ',')) {
							if (string("center-world") == optarg)
								generator.setTileOrigin(TILECENTER_IS_WORLDCENTER, TILECENTER_IS_WORLDCENTER);
							else if (string("center-map") == optarg)
								generator.setTileOrigin(TILECENTER_IS_MAPCENTER, TILECENTER_IS_MAPCENTER);
							else {
								std::cerr << "Invalid tile origin specification (" << optarg << ")" << std::endl;
								usage();
								exit(1);
							}
						}
						else {
							generator.setTileOrigin(x, y);
						}
					}
					break;
				case 'G':
					if (long_options[option_index].name[0] == 'f') {
						// '--forcegeometry'
						// Old behavior - for compatibility.
						generator.setShrinkGeometry(false);
						if (!foundGeometrySpec)
							generator.setBlockGeometry(true);
					}
					else {
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
						int p1, p2, p3, p4;
						char c;
						iss >> p1 >> c >> p2;
						if (!iss.fail() && c == 'x' && iss.eof()) {
							p3 = -(p1 / 2);
							p4 = -(p2 / 2);
						}
						else {
							char s3, s4;
							iss >> s3 >> p3 >> s4 >> p4;
							// accept +-23 as well (for ease of use)
							if ((s3 != '+'  && s3 != '-') || (s4 != '+' && s4 != '-'))
								c = 0;	// Causes an 'invalid geometry' message
							if (s3 == '-') p3 = -p3;
							if (s4 == '-') p4 = -p4;
							if (long_options[option_index].name[0] == 'c'
								&& long_options[option_index].name[1] == 'e') {
								// option 'centergeometry'
								p3 -= p1 / 2;
								p4 -= p2 / 2;
							}
						}
						if (iss.fail() || (c != ':' && c != 'x')) {
							std::cerr << "Invalid geometry specification '" << optarg << "'" << std::endl;
							usage();
							exit(1);
						}
						if ((c == ':' && (p3 < 1 || p4 < 1))
							|| (c == 'x' && (p1 < 1 || p2 < 1))) {
							std::cerr << "Invalid geometry (width and/or heigth is zero or negative)" << std::endl;
							usage();
							exit(1);
							}
						if (c == ':')
							generator.setGeometry(p1, p2, p3, p4);
						if (c == 'x')
							generator.setGeometry(p3, p4, p1, p2);

						if (!foundGeometrySpec && long_options[option_index].name[0] == 'g') {
							// Compatibility when using the option 'geometry'
							generator.setBlockGeometry(true);
							generator.setShrinkGeometry(true);
						}
						foundGeometrySpec = true;
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
		if (!colorsFile.empty()) {
			generator.parseColorsFile(colorsFile);
		}
		else {
			bool colorsFound = false;
			char *homedir;
			colorsFile = input + PATH_SEPARATOR +  "colors.txt";

			try {
				generator.parseColorsFile(colorsFile);
				colorsFound = true;
			} catch (std::runtime_error e) {
				// Ignore failure to locate world-specific colors file
			}

			if (!colorsFound && (homedir = getenv("HOME"))) {
				colorsFile = string(homedir) + PATH_SEPARATOR + ".minetest" + PATH_SEPARATOR + "colors.txt";
				try {
					generator.parseColorsFile(colorsFile);
					colorsFound = true;
				} catch (std::runtime_error e) {
					// Ignore failure to locate user private colors file
				}
			}

			// TODO: look for system-wide colors file (?) (e.g. /usr/share/games/minetest/colors.txt)
			// (location should be subject to a build-time configuration of the installation directory)

			if (!colorsFound) {
				try {
					generator.parseColorsFile("colors.txt");
					// I hope this is not obnoxious to windows users ?
					cerr	<< "Warning: Using colors.txt in current directory as a last resort." << std::endl
						<< "         Preferably, store the colors file in the world directory" << std::endl;
					if (homedir)
						cerr	<< "         or in the private minetest directory ($HOME/.minetest)." << std::endl;
					cerr	<< "         It can also be specified on the command-line" << std::endl;
				} catch(std::runtime_error e) {
					throw std::runtime_error("Failed to find or failed to open a colors.txt file.");
				}
			}
		}

		generator.generate(input, output);
	} catch(std::runtime_error e) {
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return 1;
	} catch(ZlibDecompressor::DecompressError e) {
		std::cout<<"Block decompression failure: "<<e.message<<std::endl;
		return 1;
	}
	return 0;
}

