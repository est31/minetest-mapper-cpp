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
		for (int j=0; j<m_width; j++) {
			m_pixelAttributes[i][j].m_a=0;
			m_pixelAttributes[i][j].next16Empty = (j - 1) % 16 == 0;
		}
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
			if (m_pixelAttributes[y][x].next16Empty) {
				x += 15;
				continue;
			}
			if (!m_pixelAttributes[y][x].is_valid() || !m_pixelAttributes[y - 1][x].is_valid()) {
				x++;
				continue;
			}
			if (!m_pixelAttributes[y][x - 1].is_valid())
				continue;
			if (!m_pixelAttributes[y][x].m_a)
				continue;
			double h = m_pixelAttributes[y][x].m_h;
			double h1 = m_pixelAttributes[y][x - 1].m_a ? m_pixelAttributes[y][x - 1].m_h : h;
			double h2 = m_pixelAttributes[y - 1][x].m_a ? m_pixelAttributes[y - 1][x].m_h : h;
			double d = (h - h1) + (h - h2);
			if (d > 3) {
				d = 3;
			}
			d = d * 12 / 255;
			#define pixel (m_pixelAttributes[y][x])
			//PixelAttribute &pixel = m_pixelAttributes[y][x];
			if (drawAlpha)
				d = d * (1 - pixel.m_t);
			pixel.m_r = colorSafeBounds(pixel.m_r + d);
			pixel.m_g = colorSafeBounds(pixel.m_g + d);
			pixel.m_b = colorSafeBounds(pixel.m_b + d);
			#undef pixel
		}
	}
	m_firstUnshadedY = y - yCoord2Line(0);
}

void PixelAttribute::mixUnder(const PixelAttribute &p, bool darkenHighAlpha)
{
	if (!is_valid() || m_a == 0) {
		if (!is_valid() || p.m_a != 0) {
			m_n = p.m_n;
			m_r = p.m_r;
			m_g = p.m_g;
			m_b = p.m_b;
			m_a = p.m_a;
			m_t = 0;
		}
		m_h = p.m_h;
	}
	else {
		int prev_alpha = alpha();
		m_r = (m_a * m_r + p.m_a * (1 - m_a) * p.m_r);
		m_g = (m_a * m_g + p.m_a * (1 - m_a) * p.m_g);
		m_b = (m_a * m_b + p.m_a * (1 - m_a) * p.m_b);
		m_a = (m_a + (1 - m_a) * p.m_a);
		if (p.m_a != 1)
			m_t = (m_t + p.m_t) / 2;
		m_h = p.m_h;
		if (prev_alpha >= 254 && p.alpha() < 255 && darkenHighAlpha) {
			// Darken
			// Parameters make deep water look good :-)
			// (maybe this setting should be per-node-type, and obtained from the colors file ?)
			m_r = m_r * 0.95;
			m_g = m_g * 0.95;
			m_b = m_b * 0.95;
			if (p.m_a != 1)
				m_t = (m_t + p.m_t) / 2;
		}
	}

}

