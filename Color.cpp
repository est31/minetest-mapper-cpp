
#include <cctype>
#include <stdexcept>
#include <cstdlib>
#include <map>
#include <string>
#include <cctype>
#include "Color.h"

class ColorTable : public std::map<std::string, Color>
{
public:
	ColorTable(void)
	{
	map<std::string, Color> &table = *this;
	table["white"] =	Color(0xff, 0xff, 0xff);
	table["black"] =	Color(0, 0, 0);
	table["gray"] =		Color(0x7f, 0x7f, 0x7f);
	table["grey"] =		Color(0x7f, 0x7f, 0x7f);

	table["red"] =		Color(0xff, 0, 0);
	table["green"] =	Color(0, 0xff, 0);
	table["blue"] =		Color(0, 0, 0xff);

	table["yellow"] =	Color(0xff, 0xff, 0);
	table["magenta"] =	Color(0xff, 0, 0xff);
	table["fuchsia"] =	Color(0xff, 0, 0xff);
	table["cyan"] =		Color(0, 0xff, 0xff);
	table["aqua"] =		Color(0, 0xff, 0xff);

	table["orange"] =	Color(0xff, 0x7f, 0);
	table["chartreuse"] =	Color(0x7f, 0xff, 0);
	table["pink"] =		Color(0xff, 0, 0x7f);
	table["violet"] =	Color(0x7f, 0, 0xff);
	table["springgreen"] =	Color(0, 0xff, 0x7f);
	table["azure"] =	Color(0, 0x7f, 0xff);

	table["brown"] =	Color(0x7f, 0x3f, 0);
	}
};
ColorTable colorTable;


// alpha:
//	 0: don't expect/allow alpha
//	 1: allow alpha; defaults to 255 (0xff)
//	-1: allow alpha but ignore - set to to 255 (0xff)
Color::Color(std::string color, int alpha)
{
	if (color[0] != '#') {
		int l = color.length();
		for (int i = 0; i < l; i++)
			color[i] = tolower(color[i]);
		if (colorTable.count(color) > 0) {
			*this = colorTable[color];
			return;
		}
		throw std::runtime_error(std::string("Symbolic color '") + color + "' not known, or color does not begin with #");
	}
	if (std::string::npos != color.find_first_not_of("0123456789abcdefABCDEF",1)) {
		throw std::runtime_error("Color value has invalid digits (expected: [0-9a-zA-Z])");
	}
	if (alpha) {
		if (color.length() != 4 && color.length() != 5 && color.length() != 7 && color.length() != 9)
			throw std::runtime_error("Color not in the expected format (#xxx, #xxxx, #xxxxxx or #xxxxxxxx)");
	}
	else {
		if (color.length() == 5 || color.length() == 9)
			throw std::runtime_error("Color not in the expected format (#xxx or #xxxxxx) - alpha not allowed");
		if (color.length() != 4 && color.length() != 7)
			throw std::runtime_error("Color not in the expected format (#xxx or #xxxxxx)");
	}
	unsigned col = strtoul(color.c_str() + 1, NULL, 16);
	if (color.length() < 6) {
		if (color.length() == 5)
			a = (col >> 12) & 0x0f;
		else
			a = 0x0f;
		a |= a << 4;
		r = (col >> 8) & 0x0f;
		r |= r << 4;
		g = (col >> 4) & 0x0f;
		g |= g << 4;
		b = (col >> 0) & 0x0f;
		b |= b << 4;
	}
	else {
		if (color.length() == 9)
			a = (col >> 24) & 0xff;
		else
			a = 0xff;
		r = (col >> 16) & 0xff;
		g = (col >> 8) & 0xff;
		b = (col >> 0) & 0xff;
	}
	if (alpha <= 0)
		a = 0xff;
}

