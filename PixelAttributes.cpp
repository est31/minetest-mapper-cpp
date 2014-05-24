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
	m_pixelAttributes(0)
{
}

PixelAttributes::~PixelAttributes()
{
	freeAttributes();
}

void PixelAttributes::setParameters(int width, int lines)
{
	freeAttributes();
	m_width = width + 1; // 1px gradient calculation
	m_previousLine = 0;
	m_firstLine = 1;
	m_lastLine = m_firstLine + lines - 1;
	m_emptyLine = m_lastLine + 1;
	m_lineCount = m_emptyLine + 1;
	m_firstY = 0;
	m_lastY = -1;
	m_firstUnshadedY = 0;

	m_pixelAttributes = new PixelAttribute *[m_lineCount];
	if (!m_pixelAttributes)
		throw std::runtime_error("Failed to allocate memory for PixelAttributes");

	for (int i = 0; i < m_lineCount; ++i) {
		m_pixelAttributes[i] = new PixelAttribute[m_width];
		if (!m_pixelAttributes[i])
			throw std::runtime_error("Failed to allocate memory for PixelAttributes");
	}
	for (int i=0; i<m_lineCount; i++)
		for (int j=0; j<m_width; j++)
			m_pixelAttributes[i][j].a=0;
}

void PixelAttributes::scroll(int keepY)
{
	int scroll = keepY - m_firstY;
	if (scroll > 0) {
		int i;
		for (i = m_previousLine; i + scroll <= m_lastLine; i++) {
			PixelAttribute *tmp;
			tmp = m_pixelAttributes[i];
			m_pixelAttributes[i] = m_pixelAttributes[i + scroll];
			m_pixelAttributes[i + scroll] = tmp;
		}

		size_t lineLength = m_width * sizeof(PixelAttribute);
		for (; i <= m_lastLine; ++i) {
			memcpy(m_pixelAttributes[i], m_pixelAttributes[m_emptyLine], lineLength);
		}

		m_firstY += scroll;
		m_firstUnshadedY -= scroll;
		if (m_firstUnshadedY < m_firstY) m_firstUnshadedY = m_firstY;
	}
}

void PixelAttributes::freeAttributes()
{
	if (m_pixelAttributes) {
		for (int i = 0; i < m_lineCount; ++i) {
			if (m_pixelAttributes[i] != 0) {
				delete[] m_pixelAttributes[i];
			}
		}
		delete[] m_pixelAttributes;
		m_pixelAttributes = 0;
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
	int y;
	for (y = yCoord2Line(m_firstUnshadedY); y <= yCoord2Line(m_lastY); y++) {
		for (int x = 1; x < m_width; x++) {
			if (!m_pixelAttributes[y][x].is_valid() || !m_pixelAttributes[y - 1][x].is_valid() || !m_pixelAttributes[y][x - 1].is_valid())
				continue;
			double h = m_pixelAttributes[y][x].h;
			double h1 = m_pixelAttributes[y][x - 1].h;
			double h2 = m_pixelAttributes[y - 1][x].h;
			double d = (h - h1) + (h - h2);
			if (d > 3) {
				d = 3;
			}
			d = d * 12 / 255;
			if (drawAlpha)
				d = d * (1 - m_pixelAttributes[y][x].t);
			PixelAttribute &pixel = m_pixelAttributes[y][x];
			pixel.r = colorSafeBounds(pixel.r + d);
			pixel.g = colorSafeBounds(pixel.g + d);
			pixel.b = colorSafeBounds(pixel.b + d);
		}
	}
	m_firstUnshadedY = y - yCoord2Line(0);
}

void PixelAttribute::mixUnder(const PixelAttribute &p)
{
	int prev_alpha = alpha();
	if (!is_valid() || a==0) {
		r = p.r;
		g = p.g;
		b = p.b;
		a = p.a;
		t = 0;
		h = p.h;
	}
	else {
		r = (a * r + p.a * (1 - a) * p.r);
		g = (a * g + p.a * (1 - a) * p.g);
		b = (a * b + p.a * (1 - a) * p.b);
		a = (a + (1 - a) * p.a);
		if (p.a != 1)
			t = (t + p.t) / 2;
		h = p.h;
	}
	if (prev_alpha >= 254 && p.alpha() < 255) {
		// Darken
		// Parameters make deep water look good :-)
		r = r * 0.95;
		g = g * 0.95;
		b = b * 0.95;
		if (p.a != 1)
			t = (t + p.t) / 2;
	}

}

