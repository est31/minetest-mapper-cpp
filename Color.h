
#ifndef COLOR_H
#define COLOR_H

struct Color {
	Color(): r(0xFF), g(0xFF), b(0xFF), a(0) {};
	Color(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b), a(0xFF) {};
	Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a): r(r), g(g), b(b), a(a) {};
	Color &operator=(const Color &c);
	unsigned to_uint() const { return (unsigned(a) << 24) + (unsigned(r) << 16) + (unsigned(g) << 8) + unsigned(b); }
	//libgd treats 255 as transparent, and 0 as opaque ...
	int to_libgd() const { return ((0xff - int(a)) << 24) + (int(r) << 16) + (int(g) << 8) + int(b); }
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

struct ColorEntry {
	ColorEntry(): r(0), g(0), b(0), a(0), t(0) {};
	ColorEntry(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t t): r(r), g(g), b(b), a(a), t(t) {};
	inline Color to_color() const { return Color(r, g, b, a); }
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
	uint8_t t;
};

inline Color &Color::operator=(const Color &c)
{
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
	return *this;
}

#endif // COLOR_H
