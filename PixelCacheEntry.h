
#ifndef PIXELCACHEINFO_H
#define PIXELCACHEINFO_H

#include<cmath>
#include"Color.h"

struct PixelCacheEntry {
	PixelCacheEntry();
	PixelCacheEntry(unsigned int color, double height=NAN);
	PixelCacheEntry(const Color &color, double height=NAN);
	PixelCacheEntry(const ColorEntry &entry, double height=NAN);
	Color to_color(void) const;
	ColorEntry to_colorEntry(void) const;
	void reset(void);
	void mixUnder(const PixelCacheEntry &p);
	uint8_t red(void) const { return int(r*255+0.5); }
	uint8_t green(void) const { return int(g*255+0.5); }
	uint8_t blue(void) const { return int(b*255+0.5); }
	uint8_t alpha(void) const { return int(a*255+0.5); }
	uint8_t thicken(void) const { return int(t*255+0.5); }
	int height(void) const { return int(h+0.5); }

	double r;
	double g;
	double b;
	double a;
	double t;
	double h;
	int n;
};

inline PixelCacheEntry::PixelCacheEntry() :
	r(0), g(0), b(0), a(0), t(0), h(NAN), n(0)
{
}

inline PixelCacheEntry::PixelCacheEntry(unsigned int color, double height) :
	r(((color>>16)&0xff)/255.0), g(((color>>8)&0xff)/255.0), b((color&0xff)/255.0),
	a(((color>>24)&0xff)/255.0), t(0), h(height), n(1)
{
}

inline PixelCacheEntry::PixelCacheEntry(const Color &color, double height) :
	r(color.r/255.0), g(color.g/255.0), b(color.b/255.0),
	a(color.a/255.0), t(0), h(height), n(1)
{
}

inline PixelCacheEntry::PixelCacheEntry(const ColorEntry &entry, double height) :
	r(entry.r/255.0), g(entry.g/255.0), b(entry.b/255.0),
	a(entry.a/255.0), t(entry.t/255.0), h(height), n(1)
{
}

inline Color PixelCacheEntry::to_color(void) const
{
	return Color(int(r*255+0.5),int(g*255+0.5),int(b*255+0.5),int(a*255+0.5));
}

inline ColorEntry PixelCacheEntry::to_colorEntry(void) const
{
	return ColorEntry(int(r*255+0.5),int(g*255+0.5),int(b*255+0.5),int(a*255+0.5),int(t*255+0.5));
}

inline void PixelCacheEntry::reset(void)
{
	r=0; g=0; b=0; a=0; t=0; h=NAN; n=0;
}

inline void PixelCacheEntry::mixUnder(const PixelCacheEntry &p)
{
	int prev_alpha = alpha();
	if (!n || a==0) {
		r = p.r;
		g = p.g;
		b = p.b;
		a = p.a;
		t = p.t;
		h = p.h;
		n = p.n;
	}
	else {
		// Regular mix
		r = (a * r + p.a * (1 - a) * p.r);
		g = (a * g + p.a * (1 - a) * p.g);
		b = (a * b + p.a * (1 - a) * p.b);
		a = (a + (1 - a) * p.a);
		t = (t + p.t) / 2;
		h = p.h;
	}
	if (prev_alpha == 255 && p.alpha() < 255) {
		// Darken
		// Parameters make deep water look good :-)
		r = r * (0.85 + 0.1 * (1 - p.a));
		g = g * (0.85 + 0.1 * (1 - p.a));
		b = b * (0.85 + 0.1 * (1 - p.a));
	}
}

#endif // PIXELCACHEINFO_H
