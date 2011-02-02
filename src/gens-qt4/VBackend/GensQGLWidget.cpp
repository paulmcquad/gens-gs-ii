/***************************************************************************
 * gens-qt4: Gens Qt4 UI.                                                  *
 * GensQGLWidget.cpp: QGLWidget subclass.                                  *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville.                      *
 * Copyright (c) 2003-2004 by Stéphane Akhoun.                             *
 * Copyright (c) 2008-2010 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include <config.h>

#include "GensQGLWidget.hpp"
#include "gqt4_main.hpp"

// C includes.
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// LibGens includes.
#include "libgens/MD/VdpRend.hpp"
#include "libgens/Util/Timing.hpp"
#include "libgens/MD/EmuMD.hpp"

// LOG_MSG() subsystem.
#include "libgens/macros/log_msg.h"

// Win32 requires GL/glext.h for OpenGL 1.2/1.3.
// TODO: Check the GL implementation to see what functionality is available at runtime.
#ifdef _WIN32
#include <GL/glext.h>
#endif

// Qt includes.
#include <QtCore/QVector>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>

// KeyHandlerQt.
#include "Input/KeyHandlerQt.hpp"

// gqt4_main.hpp has GensConfig.
#include "gqt4_main.hpp"

namespace GensQt4
{

GensQGLWidget::GensQGLWidget(QWidget *parent)
	: QGLWidget(QGLFormat(QGL::NoAlphaChannel | QGL::NoDepthBuffer), parent)
{
	m_tex = 0;
	
	// Initialize the OSD variables.
	m_texOsd = 0;
	m_glListOsd = 0;
	
	// Initialize the preview image texture variable.
	m_texPreview = NULL;
	
	// Accept keyboard focus.
	setFocusPolicy(Qt::StrongFocus);
	
	// Initialize mouse tracking.
	// TODO: Only do this if a Mega Mouse is connected.
	// TODO: IoMegaMouse doesn't work right.
#if 0
	setMouseTracking(true);
#endif
	
	// Connect signals from GensConfig.
	// TODO: Reconnect signals if GensConfig is deleted/recreated.
	// TODO: OSD colors.
	connect(gqt4_config, SIGNAL(osdFpsEnabled_changed(bool)),
		this, SLOT(osdFpsEnabled_changed_slot(bool)));
	connect(gqt4_config, SIGNAL(osdFpsColor_changed(const QColor&)),
		this, SLOT(osdFpsColor_changed_slot(const QColor&)));
	connect(gqt4_config, SIGNAL(osdMsgEnabled_changed(bool)),
		this, SLOT(osdMsgEnabled_changed_slot(bool)));
	connect(gqt4_config, SIGNAL(osdMsgColor_changed(const QColor&)),
		this, SLOT(osdMsgColor_changed_slot(const QColor&)));
}

GensQGLWidget::~GensQGLWidget()
{
	if (m_tex > 0)
	{
		glDeleteTextures(1, &m_tex);
		m_tex = 0;
	}
	
	if (m_texOsd > 0)
	{
		deleteTexture(m_texOsd);
		m_texOsd = 0;
	}
	
	if (m_glListOsd > 0)
	{
		glDeleteLists(m_glListOsd, 1);
		m_glListOsd = 0;
	}
	
	delete m_texPreview;
	m_texPreview = NULL;
	
	// Shut down the OpenGL Shader Manager.
	m_shaderMgr.end();
}


/**
 * reallocTexture(): (Re-)Allocate the OpenGL texture.
 */
void GensQGLWidget::reallocTexture(void)
{
	if (m_tex > 0)
		glDeleteTextures(1, &m_tex);
	
	// Create and initialize a GL texture.
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &m_tex);
	glBindTexture(GL_TEXTURE_2D, m_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	// GL filtering.
	// TODO: Make this customizable!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	// Determine the texture format and type.
	m_lastBpp = LibGens::VdpRend::m_palette.bpp();
	switch (m_lastBpp)
	{
		case LibGens::VdpPalette::BPP_15:
			m_colorComponents = 4;
			m_texFormat = GL_BGRA;
			m_texType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
			break;
		
		case LibGens::VdpPalette::BPP_16:
			m_colorComponents = 3;
			m_texFormat = GL_RGB;
			m_texType = GL_UNSIGNED_SHORT_5_6_5;
			break;
		
		case LibGens::VdpPalette::BPP_32:
		default:
			m_colorComponents = 4;
			m_texFormat = GL_BGRA;
			m_texType = GL_UNSIGNED_BYTE;
			break;
	}
	
	// Allocate a memory buffer to use for texture initialization.
	// This will ensure that the entire texture is initialized to black.
	// (This fixes garbage on the last column when using the Fast Blur shader.)
	const size_t texSize = (512*256*(m_lastBpp == LibGens::VdpPalette::BPP_32 ? 4 : 2));
	void *texBuf = calloc(1, texSize);
	
	// Allocate the texture.
	glTexImage2D(GL_TEXTURE_2D, 0,
		     m_colorComponents,
		     512, 256,	// 512x256 (320x240 rounded up to nearest powers of two)
		     0,		// No border.
		     m_texFormat, m_texType, texBuf);
	
	// Free the temporary texture buffer.
	free(texBuf);
	
	// Disable TEXTURE_2D.
	glDisable(GL_TEXTURE_2D);
	
	// Texture is dirty.
	m_vbDirty = true;
}


/**
 * reallocTexOsd(): (Re-)Allocate the OSD texture.
 */
void GensQGLWidget::reallocTexOsd(void)
{
	if (m_texOsd > 0)
		deleteTexture(m_texOsd);
	
	// Load the OSD texture.
	// TODO: Handle the case where the image isn't found.
	QImage imgOsd = QImage(":/gens/vga-charset");
	
	if (imgOsd.colorCount() > 0)
	{
		// Image uses a palette. Apply a color key.
		// Background color (black) will be changed to transparent.
		// TODO: Allow all 0x000000, or just opaque 0xFF000000?
		
		QVector<QRgb> colorTable = imgOsd.colorTable();
		for (int i = 0; i < colorTable.size(); i++)
		{
			if (colorTable[i] == 0xFF000000)
			{
				// Found black. Make it transparent.
				colorTable[i] = 0x00000000;
				break;
			}
		}
		
		imgOsd.setColorTable(colorTable);
	}
	else if (!imgOsd.hasAlphaChannel())
	{
		// Image doesn't have an alpha channel.
		// Convert it to 32-bit RGBA and apply a color key.
		// Background color (black) will be changed to transparent.
		imgOsd = imgOsd.convertToFormat(QImage::Format_ARGB32, Qt::ColorOnly);
		
		// TODO: Byte ordering on big-endian systems.
		// TODO: Allow all 0x000000, or just opaque 0xFF000000?
		// TODO: Handle images that don't have a color table.
		// TODO: Images with odd widths will not have the last column updated properly.
		
		for (int y = 0; y < imgOsd.height(); y++)
		{
			QRgb *scanline = reinterpret_cast<QRgb*>(imgOsd.scanLine(y));
			for (int x = imgOsd.width() / 2; x != 0; x--, scanline += 2)
			{
				if (*scanline == 0xFF000000)
					*scanline = 0x00000000;
				if (*(scanline+1) == 0xFF000000)
					*(scanline+1) = 0x00000000;
			}
			
			// TODO: Make this work on images with odd widths?
		}
	}
	
	// Bind the OSD image to a texture.
	m_texOsd = bindTexture(imgOsd);
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_texOsd);
	
	// Set texture parameters.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	// GL filtering.
	// TODO: Make this customizable!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	glDisable(GL_TEXTURE_2D);
}


/**
 * initializeGL(): Called when GL is initialized.
 */
void GensQGLWidget::initializeGL(void)
{
	// OpenGL initialization.
	
	// Disable various OpenGL functionality.
	glDisable(GL_DEPTH_TEST);	// Depth buffer.
	glDisable(GL_BLEND);		// Blending.
	
	// Enable face culling.
	// This disables drawing the backsides of polygons.
	// Also, set the front face to GL_CW.
	// TODO: GL_CCW is default; rework everything to use CCW instead?
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	
	// Initialize the OpenGL Shader Manager.
	m_shaderMgr.init();

	// Initialize the GL viewport and projection.
	resizeGL(320, 240);
	
	// Allocate the texture.
	reallocTexture();
	
	// Allocate the OSD texture.
	reallocTexOsd();
	
	// Generate a display list for the OSD.
	m_glListOsd = glGenLists(1);
}


void GensQGLWidget::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	
	// Set the OpenGL projection.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	// Aspect ratio constraint.
	bool aspect_constraint = true;	// TODO: Make this configurable.
	if (!aspect_constraint)
	{
		// No aspect ratio constraint.
		glOrtho(-1, 1, -1, 1, -1, 1);
	}
	else
	{
		// Aspect ratio constraint.
		if ((width * 3) > (height * 4))
		{
			// Image is wider than 4:3.
			const double ratio = ((double)(width * 3) / (double)(height * 4));
			glOrtho(-ratio, ratio, -1, 1, -1, 1);
		}
		else if ((width * 3) < (height * 4))
		{
			// Image is taller than 4:3.
			const double ratio = ((double)(height * 4) / (double)(width * 3));
			glOrtho(-1, 1, -ratio, ratio, -1, 1);
		}
		else
		{
			// Image has the correct aspect ratio.
			glOrtho(-1, 1, -1, 1, -1, 1);
		}
	}
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


void GensQGLWidget::paintGL(void)
{
	/**
	 * bFromMD: If this is true after all effects are applied,
	 * use LibGens::VDP_Rend::MD_Screen[] directly.
	 * Otherwise, use m_intScreen[].
	 */
	bool bFromMD = true;
	
	glClearColor(1.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	if (m_vbDirty)
	{
		// MD_Screen is dirty.
		
		// Check if the Bpp has changed.
		if (LibGens::VdpRend::m_palette.bpp() != m_lastBpp)
		{
			// Bpp has changed. Reallocate the texture.
			// TODO: Recalculate palettes?
			reallocTexture();
		}
		
		/** START: Apply effects. **/
		if (isRunning())
		{
			// Emulation is running. Check if any effects should be applied.
			
			// If Fast Blur is enabled, update the Fast Blur effect.
			// NOTE: Shader version is only used if we're not paused.
			if (fastBlur() && (isPaused() || !m_shaderMgr.hasFastBlur()))
			{
				updateFastBlur(bFromMD);
				bFromMD = false;
			}
			
			// If emulation is paused, update the pause effect.
			if (isPaused() && !m_shaderMgr.hasPaused())
			{
				// Paused, but no shader is available.
				// Apply the effect in software.
				updatePausedEffect(bFromMD);
				bFromMD = false;
			}
		}
		
		// Determine which screen buffer should be used for video output.
		GLvoid *screen = (bFromMD ? (GLvoid*)LibGens::VdpRend::MD_Screen.u16 : (GLvoid*)m_intScreen);
		
		/** END: Apply effects. **/
		
		// Bind the texture.
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_tex);
		
		// (Re-)Upload the texture.
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 336);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 8);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
		
		glTexSubImage2D(GL_TEXTURE_2D, 0,
				0, 0,		// x/y offset
				320, 240,	// width/height
				m_texFormat, m_texType, screen);
		
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
		
		// Texture is no longer dirty.
		m_vbDirty = false;
	}
	else
	{
		// MD Screen isn't dirty.
		// Simply bind the texture.
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_tex);
	}
	
	// Enable shaders, if necessary.
	if (isRunning())
	{
		if (m_shaderMgr.hasFastBlur() && fastBlur() && !isPaused())
		{
			// Enable the Fast Blur shader.
			// NOTE: Shader version is only used if we're not paused.
			m_shaderMgr.setFastBlur(true);
		}
		else if (m_shaderMgr.hasPaused() && isPaused())
		{
			// Enable the Paused shader.
			m_shaderMgr.setPaused(true);
		}
	}
	
	// Determine the texture coordinates.
	// TODO: Make a separate function to recalculate stretch mode coordinates.
	// TODO: This function would need to be called if:
	// * MD resolution is changed.
	// * Stretch mode is changed.
	double img_dx, img_dy, img_dw, img_dh;
	const double tex_w = 512.0, tex_h = 256.0;
	
	// Default to no stretch.
	img_dx = 0.0;
	img_dy = 0.0;
	img_dw = (320.0 / tex_w);
	img_dh = (240.0 / tex_h);
	
	if (m_stretchMode == STRETCH_H || m_stretchMode == STRETCH_FULL)
	{
		// Horizontal stretch.
		int h_pix_begin = LibGens::VdpIo::GetHPixBegin();
		if (h_pix_begin > 0)
		{
			// Less than 320 pixels wide.
			// Adjust horizontal stretch.
			img_dx = ((double)h_pix_begin / tex_w);
			img_dw -= img_dx;
		}
	}
	
	if (m_stretchMode == STRETCH_V || m_stretchMode == STRETCH_FULL)
	{
		// Vertical stretch.
		int v_pix = (240 - LibGens::VdpIo::GetVPix());
		if (v_pix > 0)
		{
			// Less than 240 pixels tall.
			v_pix /= 2;
			img_dy = ((double)v_pix / tex_h);
			img_dh -= img_dy;
		}
	}
	
	// Draw the texture.
	glBindTexture(GL_TEXTURE_2D, m_tex);
	glBegin(GL_QUADS);
	glTexCoord2d(img_dx, img_dy);
	glVertex2i(-1, 1);
	glTexCoord2d(img_dw, img_dy);
	glVertex2i(1, 1);
	glTexCoord2d(img_dw, img_dh);
	glVertex2i(1, -1);
	glTexCoord2d(img_dx, img_dh);
	glVertex2i(-1, -1);
	glEnd();
	
	// Disable shaders, if necessary.
	if (isRunning())
	{
		if (m_shaderMgr.hasFastBlur() && fastBlur() && !isPaused())
		{
			// Disable the Fast Blur shader.
			// NOTE: Shader version is only used if we're not paused.
			m_shaderMgr.setFastBlur(false);
		}
		else if (m_shaderMgr.hasPaused() && isPaused())
		{
			// Disable the Paused shader.
			m_shaderMgr.setPaused(false);
		}
	}
	
	// Disable 2D textures.
	glDisable(GL_TEXTURE_2D);
	
	// Display the OSD preview image.
	showOsdPreview();
	
	// Print the OSD text to the screen.
	printOsdText();
}


/**
 * printOsdText(): Print the OSD text to the screen.
 */
void GensQGLWidget::printOsdText(void)
{
	// Print text to the screen.
	// TODO:
	// * renderText() doesn't support wordwrapping.
	// * renderText() doesn't properly handle newlines.
	// * fm.boundingRect() doesn't seem to handle wordwrapping correctly, either.
	
	// TODO: Font information.
	static const int chrW = 8;
	static const int chrH = 16;
	
	if (!m_osdListDirty)
	{
		// OSD message list isn't dirty.
		// Render the display list.
		glCallList(m_glListOsd);
		return;
	}
	
	// OSD message list is dirty.
	// Create a new GL display list.
	glNewList(m_glListOsd, GL_COMPILE_AND_EXECUTE);
	
	// Set pixel matrices.
	// Assuming 320x240 for 1x text rendering.
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 320, 240, 0, -1.0f, 1.0f);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	// Trick to fix up pixel alignment.
	// See http://basic4gl.wikispaces.com/2D+Drawing+in+OpenGL
	glTranslatef(0.375f, 0.375f, 0.0f);
	
	// Enable 2D textures.
	glEnable(GL_TEXTURE_2D);
	
	// Enable GL blending.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// Bind the OSD texture.
	glBindTexture(GL_TEXTURE_2D, m_texOsd);
	glBegin(GL_QUADS);	// Start drawing quads.
	
	// Text shadow color.
	// TODO: Make this customizable?
	const QColor clShadow(0, 0, 0);
	
	// TODO: Constants for character sizes.
	int y = (240 - chrH);
	double curTime = LibGens::Timing::GetTimeD();
	
	// Check if the FPS should be drawn.
	// TODO: Integrate this with the for loop.
	if (isRunning() && !isPaused() && osdFpsEnabled())
	{
		QString sFps = QString::number(m_fpsAvg, 'f', 1);
		
		// TODO: Allow font scaling.
		y -= chrH;
		
		// TODO: Make the drop shadow optional or something.
		qglColor(clShadow);
		printOsdLine(chrW+1, y+1, sFps);
		qglColor(osdFpsColor());
		printOsdLine(chrW, y, sFps);
	}
	
	// If messages are enabled, print them on the screen.
	if (osdMsgEnabled())
	{
		// NOTE: QList internally uses an array of pointers.
		// We can use array indexing instead of iterators.
		for (int i = (m_osdList.size() - 1); i >= 0; i--)
		{
			if (curTime >= m_osdList[i].endTime)
			{
				// Message duration has elapsed.
				// Remove the message from the list.
				m_osdList.removeAt(i);
				continue;
			}
			
			const QString &msg = m_osdList[i].msg;
			
			// TODO: Allow font scaling.
			y -= chrH;
			
			// TODO: Make the drop shadow optional or something.
			qglColor(clShadow);
			printOsdLine(chrW+1, y+1, msg);
			qglColor(osdMsgColor());	// TODO: Per-message colors?
			printOsdLine(chrW, y, msg);
		}
	}
	
	// Check if there's any recording status messages.
	if (!m_osdRecList.isEmpty())
	{
		// Recording status messages are present.
		// Print at the upper-right of the screen.
		y = chrH;
		QColor clStopped(255, 255, 255);
		
		QColor clRec;
		QString sRec;
		if (isRunning())
		{
			if (isPaused())
			{
				clRec = QColor(255, 255, 255);
				sRec = QChar(0x25CF);
				sRec += QChar(0xF8FE);
			}
			else
			{
				clRec = QColor(255, 0, 0);
				sRec = QChar(0x25CF);
			}
		}
		else
		{
			clRec = QColor(255, 255, 255);
			sRec = QChar(0x25A0);
		}
		
		QString msg;
		for (int i = 0; i < m_osdRecList.size(); i++)
		{
			const RecOsd &recOsd = m_osdRecList[i];
			msg = (recOsd.isRecording ? sRec : QChar(0x25A0)) +
				recOsd.component + ' ';
			
			// Format the duration.
			int secs = (recOsd.duration / 1000);
			int mins = (secs / 60);
			secs %= 60;
			msg += QString("%1:%2").arg(mins).arg(QString::number(secs), 2, '0');
			
			// Calculate the message width.
			const int msgW = ((msg.size() + 1) * chrW);
			const int x = (320 - msgW);
			
			// TODO: Make the drop shadow optional or something.
			qglColor(clShadow);
			printOsdLine(x+1, y+1, msg);
			qglColor(recOsd.isRecording ? clRec : clStopped);	// TODO: Per-message colors?
			printOsdLine(x, y, msg);
			
			// TODO: Allow font scaling. (?)
			y += chrH;
		}
	}
	
	// We're done drawing.
	glEnd();
	
	// Reset the GL state.
	qglColor(QColor(255, 255, 255, 255));	// Reset the color.
	glDisable(GL_BLEND);			// Disable GL blending.
	glDisable(GL_TEXTURE_2D);		// Disable 2D textures.
	
	// Restore the matrices.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	// Finished creating the GL display list.
	glEndList();
	m_osdListDirty = false;		// OSD message list is no longer dirty.
}


/**
 * printOsdLine(): Print a line of text on the screen.
 * NOTE: This should ONLY be called from printOsdText()!
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param msg Line of text.
 */
void GensQGLWidget::printOsdLine(int x, int y, const QString &msg)
{
	// TODO: Font information.
	// TODO: Wordwrapping.
	static const int chrW = 8;
	static const int chrH = 16;
	
	for (int i = 0; i < msg.size(); i++, x += chrW)
	{
		uint16_t chr = msg[i].unicode();
		if (chr > 0xFF)
		{
			// Unicode characters over U+00FF are not supported right now.
			// TODO: Replacement character.
			
			// cp437 control characters (0x00-0x20) are now supported.
			// Check if this is a control character.
			// TODO: Optimize this!
			switch (chr)
			{
				case 0x263A:	chr = 0x01; break;
				case 0x263B:	chr = 0x02; break;
				case 0x2665:	chr = 0x03; break;
				case 0x2666:	chr = 0x04; break;
				case 0x2663:	chr = 0x05; break;
				case 0x2660:	chr = 0x06; break;
				case 0x2022:	chr = 0x07; break;
				case 0x2508:	chr = 0x08; break;
				case 0x25CB:	chr = 0x09; break;
				case 0x25D9:	chr = 0x0A; break;
				case 0x2642:	chr = 0x0B; break;
				case 0x2640:	chr = 0x0C; break;
				case 0x266A:	chr = 0x0D; break;
				case 0x266B:	chr = 0x0E; break;
				case 0x263C:	chr = 0x0F; break;
				case 0x25BA:	chr = 0x10; break;
				case 0x25C4:	chr = 0x11; break;
				case 0x2195:	chr = 0x12; break;
				case 0x203C:	chr = 0x13; break;
				//case 0x00B6:	chr = 0x14; break;	// This is part of cp1252...
				//case 0x00A7:	chr = 0x15; break;	// This is part of cp1252...
				case 0x25AC:	chr = 0x16; break;
				case 0x21A8:	chr = 0x17; break;
				case 0x2191:	chr = 0x18; break;
				case 0x2193:	chr = 0x19; break;
				case 0x2192:	chr = 0x1A; break;
				case 0x2190:	chr = 0x1B; break;
				case 0x221F:	chr = 0x1C; break;
				case 0x2194:	chr = 0x1D; break;
				case 0x2582:	chr = 0x1E; break;
				case 0x25BC:	chr = 0x1F; break;
				
				// VCR symbols.
				case 0x25CF:	chr = 0x80; break;	// Record. (BLACK CIRCLE)
				case 0xF8FE:	chr = 0x81; break;	// Pause. (Private Use Area)
				case 0x25A0:	chr = 0x82; break;	// Stop. (BLACK SQUARE)
				
				default:	chr = 0; break;
			}
			
			if (chr == 0)
				continue;
		}
		
		// Calculate the texture coordinates.
		// TODO: The Y coordinate seems to be inverted...
		GLfloat tx1 = ((float)(chr & 0xF) / 16.0f);
		GLfloat ty1 = 1.0 - ((float)((chr & 0xF0) >> 4) / 16.0f);
		GLfloat tx2 = (tx1 + (1.0f / 16.0f));
		GLfloat ty2 = (ty1 - (1.0f / 16.0f));
		
		// Draw the texture.
		// NOTE: glBegin() / glEnd() are called in printOsdText().
		glTexCoord2f(tx1, ty1);
		glVertex2i(x, y);
		glTexCoord2d(tx2, ty1);
		glVertex2i(x+chrW, y);
		glTexCoord2d(tx2, ty2);
		glVertex2i(x+chrW, y+chrH);
		glTexCoord2d(tx1, ty2);
		glVertex2i(x, y+chrH);
	}
}


/**
 * osd_show_preview(): Show a preview image on the OSD.
 * @param duration Duration for the preview image to appaer, in milliseconds.
 * @param img Image to show.
 */
void GensQGLWidget::osd_show_preview(int duration, const QImage& img)
{
	// Call the base function first.
	VBackend::osd_show_preview(duration, img);
	
	// Delete the preview texture to force a refresh.
	delete m_texPreview;
	m_texPreview = NULL;
}


/**
 * showOsdPreview(): Show the OSD preview image.
 */
void GensQGLWidget::showOsdPreview(void)
{
	// TODO: Duration, etc.
	if (!m_preview_show)
	{
		// Don't show the preview image.
		delete m_texPreview;
		m_texPreview = NULL;
		return;
	}
	
	// Enable 2D textures.
	glEnable(GL_TEXTURE_2D);
	
	// Show the preview image.
	if (!m_texPreview)
	{
		// Create the texture for the preview image.
		m_texPreview = new GlTex2D();
		m_texPreview->setImage(m_preview_img);
	}
	
	// Bind the texture.
	glBindTexture(GL_TEXTURE_2D, m_texPreview->tex());	
	
	// Calculate the destination coordinates.
	// TODO: Precalculate?
	// TODO: Reposition based on stretch mode and current display resolution.
	const GLdouble x1 = (-1.0 + (0.0625 * 3.0 / 4.0)), y1 = (1.0 - 0.0625);
	GLdouble x2, y2;
	
	// Calculate (x2, y2) based on stretch mode.
	if (m_stretchMode == STRETCH_H || m_stretchMode == STRETCH_FULL)
		x2 = x1 + 0.5;
	else
		x2 = x1 + std::min(((m_texPreview->img_w() / 320.0) * 0.5), 0.5);
	
	if (m_stretchMode == STRETCH_V || m_stretchMode == STRETCH_FULL)
		y2 = y1 - 0.5;
	else
		y2 = y1 - std::min(((m_texPreview->img_h() / 240.0) * 0.5), 0.5);
	
	// Draw the texture.
	// TODO: Determine where to display it and what size to use.
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0);
	glVertex2d(x1, y1);
	glTexCoord2d(m_texPreview->pow2_w(), 0);
	glVertex2d(x2, y1);
	glTexCoord2d(m_texPreview->pow2_w(), m_texPreview->pow2_h());
	glVertex2d(x2, y2);
	glTexCoord2d(0, m_texPreview->pow2_h());
	glVertex2d(x1, y2);
	glEnd();
	
	// Disable 2D textures.
	glEnable(GL_TEXTURE_2D);
}

}
