/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  25.08.2012 10:55:29
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#ifndef PIXELATTRIBUTES_H_ADZ35GYF
#define PIXELATTRIBUTES_H_ADZ35GYF

#include <limits>
#include <cmath>
#include <stdint.h>
#include <stdexcept>
#include <cassert>
#include "config.h"
#include "Color.h"

struct PixelAttribute {
	PixelAttribute(): h(NAN), t(0), a(0), r(0), g(0), b(0) {};
	PixelAttribute(const Color &color, double height);
	PixelAttribute(const ColorEntry &entry, double height);
	double h;
	double t;
	double a;
	double r;
	double g;
	double b;
	uint8_t red(void) const { return int(r*255+0.5); }
	uint8_t green(void) const { return int(g*255+0.5); }
	uint8_t blue(void) const { return int(b*255+0.5); }
	uint8_t alpha(void) const { return int(a*255+0.5); }
	uint8_t thicken(void) const { return int(t*255+0.5); }
	unsigned height(void) const { return unsigned(h+0.5); }
	Color color(void) const { return Color(red(), green(), blue(), alpha()); }

	inline bool is_valid() const { return !isnan(h); }
	PixelAttribute &operator=(const PixelAttribute &p);
	void mixUnder(const PixelAttribute &p);
};

class PixelAttributes
{
public:
	PixelAttributes();
	virtual ~PixelAttributes();
	void setParameters(int width, int lines);
	void scroll(int keepY);
	PixelAttribute &attribute(int y, int x);
	void renderShading(bool drawAlpha);
	void setLastY(int y);
	int getLastY(void) { return m_lastY; }

private:
	int yCoord2Line(int y) { return y - m_firstY + m_firstLine; }
	void freeAttributes();

private:
	int m_previousLine;
	int m_firstLine;
	int m_lastLine;
	int m_emptyLine;
	int m_lineCount;
	PixelAttribute **m_pixelAttributes;
	int m_width;
	int m_firstY;
	int m_lastY;
	int m_firstUnshadedY;
};

inline void PixelAttributes::setLastY(int y)
{
#ifdef DEBUG
	assert(y - m_firstY <= m_lastLine - m_firstLine);
#else
	if (y - m_firstY > m_lastLine - m_firstLine)
		// Not sure whether this will actually avoid a crash...
		y = m_firstY + (m_lastLine - m_firstLine);
#endif
	m_lastY = y;
}

inline PixelAttribute &PixelAttributes::attribute(int y, int x)
{
#ifdef DEBUG
	assert(yCoord2Line(y) >= m_firstLine && yCoord2Line(y) <= m_lastLine);
#else
	static PixelAttribute p;
	if (!(yCoord2Line(y) >= m_firstLine && yCoord2Line(y) <= m_lastLine))
		return p;
#endif
	return m_pixelAttributes[yCoord2Line(y)][x + 1];
}

inline PixelAttribute::PixelAttribute(const Color &color, double height) :
	h(height), t(0), a(color.a/255.0),
	r(color.r/255.0), g(color.g/255.0), b(color.b/255.0)
{
}

inline PixelAttribute::PixelAttribute(const ColorEntry &entry, double height) :
	h(height), t(entry.t/255.0), a(entry.a/255.0),
	r(entry.r/255.0), g(entry.g/255.0), b(entry.b/255.0)
{
}

inline PixelAttribute &PixelAttribute::operator=(const PixelAttribute &p)
{
	h = p.h;
	t = p.t;
	a = p.a;
	r = p.r;
	g = p.g;
	b = p.b;
	return *this;
}

#endif /* end of include guard: PIXELATTRIBUTES_H_ADZ35GYF */

