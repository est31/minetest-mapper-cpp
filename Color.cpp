
#include <cctype>
#include <stdexcept>
#include "Color.h"

// alpha:
//	 0: don't expect/allow alpha
//	 1: allow alpha; defaults to 255 (0xff)
//	-1: allow alpha but ignore - set to to 255 (0xff)
Color::Color(const std::string &color, int alpha)
{
	if (color[0] != '#') {
		throw std::runtime_error("Color does not begin with #");
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

