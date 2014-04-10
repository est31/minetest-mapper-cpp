
#ifndef COLOR_H
#define COLOR_H

struct Color {
	Color(): r(0xFF), g(0xFF), b(0xFF), a(0) {};
	Color(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b), a(0xFF) {};
	Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a): r(r), g(g), b(b), a(a) {};
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

inline int rgb2int(uint8_t r, uint8_t g, uint8_t b, uint8_t a=0xFF)
{
	return (a << 24) + (r << 16) + (g << 8) + b;
}

//libgd treats 255 as transparent, and 0 as opaque ...
inline int rgb2libgd(uint8_t r, uint8_t g, uint8_t b, uint8_t a=0xFF)
{
	return rgb2int(r, g, b, 0xff-a);
}

inline int color2int(Color c)
{
    return rgb2int(c.r, c.g, c.b, c.a);
}

inline int color2libgd(Color c)
{
    return rgb2libgd(c.r, c.g, c.b, c.a);
}

#endif // COLOR_H
