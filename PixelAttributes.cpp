/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  25.08.2012 10:55:27
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include "PixelAttributes.h"

using namespace std;

PixelAttributes::PixelAttributes():
	m_width(0)
{
	for (size_t i = 0; i < LineCount; ++i) {
		m_pixelAttributes[i] = 0;
	}
}

PixelAttributes::~PixelAttributes()
{
	freeAttributes();
}

void PixelAttributes::setWidth(int width)
{
	freeAttributes();
	m_width = width + 1; // 1px gradient calculation
	for (size_t i = 0; i < LineCount; ++i) {
		m_pixelAttributes[i] = new PixelAttribute[m_width];
	}
}

void PixelAttributes::scroll()
{
	size_t lineLength = m_width * sizeof(PixelAttribute);
	memcpy(m_pixelAttributes[PreviousLine], m_pixelAttributes[LastLine], lineLength);
	for (size_t i = 1; i < LineCount - 1; ++i) {
		memcpy(m_pixelAttributes[i], m_pixelAttributes[EmptyLine], lineLength);
	}
}

void PixelAttributes::freeAttributes()
{
	for (size_t i = 0; i < LineCount; ++i) {
		if (m_pixelAttributes[i] != 0) {
			delete[] m_pixelAttributes[i];
			m_pixelAttributes[i] = 0;
		}
	}
}


static inline double colorSafeBounds(double color)
{
	if (color > 1) {
		return 1;
	}
	else if (color < 0) {
		return 0;
	}
	else {
		return color;
	}
}


void PixelAttributes::renderShading(bool drawAlpha)
{
	for (int z = FirstLine; z <= LastLine; z++) {
		for (int x = 1; x < m_width; x++) {
			if (!m_pixelAttributes[z][x].valid_height() || !m_pixelAttributes[z - 1][x].valid_height() || !m_pixelAttributes[z][x - 1].valid_height())
				continue;
			double y = m_pixelAttributes[z][x].h;
			double y1 = m_pixelAttributes[z][x - 1].h;
			double y2 = m_pixelAttributes[z - 1][x].h;
			double d = (y - y1) + (y - y2);
			if (d > 3) {
				d = 3;
			}
			d = d * 12 / 255;
			if (drawAlpha)
				d = d * (1 - m_pixelAttributes[z][x].t);
			PixelAttribute &pixel = m_pixelAttributes[z][x];
			pixel.r = colorSafeBounds(pixel.r + d);
			pixel.g = colorSafeBounds(pixel.g + d);
			pixel.b = colorSafeBounds(pixel.b + d);
		}
	}
}

void PixelAttribute::mixUnder(const PixelAttribute &p)
{
	int prev_alpha = alpha();
	if (!valid_height() || a==0) {
		r = p.r;
		g = p.g;
		b = p.b;
		a = p.a;
		t = p.t;
		h = p.h;
	}
	else {
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

