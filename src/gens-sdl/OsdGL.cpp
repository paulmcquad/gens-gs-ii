/***************************************************************************
 * gens-sdl: Gens/GS II basic SDL frontend.                                *
 * OsdGL.hpp: Onscreen Display for OpenGL.                                 *
 *                                                                         *
 * Copyright (c) 2008-2015 by David Korth.                                 *
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

#include "OsdGL.hpp"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstdio> /* REMOVE LATER */
#include <cstdlib>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

// LibGens.
#include "libgens/Util/Timing.hpp"

// OpenGL
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

// OSD font.
#include "OsdFont_VGA.hpp"

namespace GensSdl {

class OsdGLPrivate {
	public:
		OsdGLPrivate();

	private:
		// Q_DISABLE_COPY() equivalent.
		// TODO: Add GensSdl-specific version of Q_DISABLE_COPY().
		OsdGLPrivate(const OsdGLPrivate &);
		OsdGLPrivate &operator=(const OsdGLPrivate &);

	public:
		GLuint texOsd;			// OSD texture.
		GLfloat osdVertex[256][8];	// Texture vertex array.

		/**
		 * (Re-)Allocate the OSD texture.
		 */
		void reallocOsdTexture();

		/**
		 * Print a line of text on the screen.
		 * @param x X coordinate.
		 * @param y Y coordinate.
		 * @param msg Line of text. (NOTE: ASCII only right now...)
		 */
		void printLine(int x, int y, const string &msg);

		// Timer.
		LibGens::Timing timer;

		// TODO: Get this from the OSD font.
		const int chrW = 8;	// must be pow2
		const int chrH = 16;	// must be pow2

		// OSD queue.
		// NOTE: Manually allocating objects.
		struct OsdMessage {
			string msg;		// Message. (Converted to internal 8-bit charset.)
			unsigned int duration;	// Duration, in milliseconds.
			bool hasDisplayed;	// Timer starts counting down once the message has been displayed.
			uint64_t endTime;	// End time, using internal Timing object. (microseconds)
		};
		vector<OsdMessage*> osdList;

		// OpenGL Display List.
		GLuint displayList;

		// Is the OSD list dirty?
		// If so, the display list needs to be regenerated.
		// TODO: Use a display list.
		bool dirty;

		/**
		 * Check for expired messages.
		 * Expired messages will be removed from the list.
		 */
		void checkForExpiredMessages(void);

		/** Properties. **/
		bool fpsEnabled;
		bool msgEnabled;

		uint32_t fpsColor;
		uint32_t msgColor;

		static void setGLColor(uint32_t color);
};

/** OsdGLPrivate **/

OsdGLPrivate::OsdGLPrivate()
	: texOsd(0)
	, displayList(0)
	, dirty(true)
	, fpsEnabled(false)
	, msgEnabled(true)
	, fpsColor(0xFFFFFFFF)
	, msgColor(0xFFFFFFFF)
{
	// Reserve space for at least 8 OSD messages.
	osdList.reserve(8);
}

/**
 * (Re-)Allocate the OSD texture.
 */
void OsdGLPrivate::reallocOsdTexture()
{
	if (texOsd == 0) {
		// Create a texture.
		glGenTextures(1, &texOsd);
	}
	if (displayList == 0) {
		// Create a DisplayList.
		displayList = glGenLists(1);
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texOsd);

	// Set texture parameters.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Texture filtering.
	// TODO: Should we use linear filtering for the OSD text?
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glDisable(GL_TEXTURE_2D);

	// Calculate the GL vertex array.
	GLfloat *vtx = &osdVertex[0][0];
	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			vtx[0] = ((float)(x+0) / 16.0f);
			vtx[1] = ((float)(y+0) / 16.0f);
			vtx[2] = ((float)(x+1) / 16.0f);
			vtx[3] = ((float)(y+0) / 16.0f);
			vtx[4] = ((float)(x+1) / 16.0f);
			vtx[5] = ((float)(y+1) / 16.0f);
			vtx[6] = ((float)(x+0) / 16.0f);
			vtx[7] = ((float)(y+1) / 16.0f);
			vtx += 8;
		}
	}

	// Create the GL image.
	// Using GL_ALPHA.
	// TODO: Optimize this?
	uint8_t *glImage = (uint8_t*)malloc(256 * 16 * 8);
	// Converting 1bpp characters to 8bpp.
	// pitch = 8 pixels per character; 16 per line.
	const int pitch = chrW * 16;
	for (int chr = 0; chr < 256; chr++) {
		const int y_pos = (chr & ~(chrH - 1));
		const int x_pos = (chr & (chrH - 1)) * chrW;

		uint8_t *pos = &glImage[(y_pos * pitch) + x_pos];
		for (int y = 0; y < chrH; y++, pos += (pitch - chrW)) {
			uint8_t chr_data = VGA_charset_ASCII[chr][y];
			for (int x = chrW; x > 0; x--, chr_data <<= 1) {
				*pos = (chr_data & 0x80 ? 0xFF : 0);
				pos++;
			}
		}
	}

	// Upload the image data.
	glTexImage2D(GL_TEXTURE_2D, 0,
			GL_ALPHA,	// One component.
			128, 256,	// Texture size.
			0,		// No border.
			GL_ALPHA, GL_UNSIGNED_BYTE, glImage);
	free(glImage);

	// OSD is dirty.
	dirty = true;
}

/**
 * Print a line of text on the screen.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param msg Line of text. (NOTE: ASCII only right now...)
 */
void OsdGLPrivate::printLine(int x, int y, const std::string &msg)
{
	// TODO: Font information.
	const int chrW = 8;
	const int chrH = 16;

	// TODO: Wordwrapping.

	// TODO: Precalculate vertices?
	const int len = (int)msg.size();
	// TODO: Allocate once, and reallocate if a larger one is needed?
	GLint *vtx = new GLint[len * 8];
	GLfloat *txc = new GLfloat[len * 8];

	// TODO: Optimize this!
	const char *pChr = msg.c_str();
	for (int i = 0; i < (int)msg.size(); i++, x += chrW, pChr++) {
		uint8_t chr = *pChr;
		if (chr == 0)
			break;

		// Vertex coordinates.
		vtx[(i*8)+0] = x;
		vtx[(i*8)+1] = y;
		vtx[(i*8)+2] = x+chrW;
		vtx[(i*8)+3] = y;
		vtx[(i*8)+4] = x+chrW;
		vtx[(i*8)+5] = y+chrH;
		vtx[(i*8)+6] = x;
		vtx[(i*8)+7] = y+chrH;

		// Texture coordinates.
		memcpy(&txc[i*8], &osdVertex[chr][0], sizeof(osdVertex[chr]));
	}

	glVertexPointer(2, GL_INT, 0, vtx);
	glTexCoordPointer(2, GL_FLOAT, 0, txc);
	glDrawArrays(GL_QUADS, 0, len*4);

	delete[] txc;
	delete[] vtx;
}

/**
 * Check for expired messages.
 * Expired messages will be removed from the list.
 */
void OsdGLPrivate::checkForExpiredMessages(void)
{
	const uint64_t curTime = timer.getTime();
	bool isAllProcessed = true;

	for (int i = (int)osdList.size() - 1; i >= 0; i--) {
		OsdGLPrivate::OsdMessage *osdMsg = osdList[i];
		if (!osdMsg)
			continue;

		if (curTime >= osdMsg->endTime) {
			// Time has elapsed.
			// Check if the message has been displayed.
			if (osdMsg->hasDisplayed) {
				// Message has been displayed.
				// Remove the message from the list.
				delete osdList[i];
				osdList[i] = nullptr;
				dirty = true;
			} else {
				// Message has *not* been displayed.
				// Reset its end time.
				osdMsg->endTime = curTime + (osdMsg->duration * 1000);
				isAllProcessed = false;
			}
		} else {
			// Message has not been processed yet.
			isAllProcessed = false;
		}
	}

	if (isAllProcessed) {
		// TODO: Use an std::list and remove entries as they're processed?
		// All messages have been processed.
		osdList.clear();
	}
}

/**
 * Set the OpenGL color
 * @param color 32-bit ARGB color.
 */
inline void OsdGLPrivate::setGLColor(uint32_t color)
{
	// FIXME: 0xFFFFFF seems to map to (254,254,254)...
	const uint8_t a = (color >> 24) & 0xFF;
	const uint8_t r = (color >> 16) & 0xFF;
	const uint8_t g = (color >>  8) & 0xFF;
	const uint8_t b = (color >>  0) & 0xFF;
	glColor4ub(a, r, g, b);
}

/** OsdGL **/

OsdGL::OsdGL()
	: d(new OsdGLPrivate())
{ }

OsdGL::~OsdGL()
{
	end();	// TODO: Must be in a valid GL context...
	delete d;
}

/**
 * Initialize the Onscreen Display.
 * This must be called from a valid GL context.
 */
void OsdGL::init(void)
{
	// Allocate the OSD texture.
	d->reallocOsdTexture();
}

/**
 * Shut down the Onscreen Display.
 * This must be called from a valid GL context.
 */
void OsdGL::end(void)
{
	if (d->texOsd > 0) {
		glDeleteTextures(1, &d->texOsd);
		d->texOsd = 0;
	}
	if (d->displayList > 0) {
		glDeleteLists(d->displayList, 1);
		d->displayList = 0;
	}
}

/**
 * Draw the Onscreen Display.
 * This must be called from a valid GL context.
 */
void OsdGL::draw(void)
{
	if (d->osdList.empty()) {
		// No OSD messages.
		// TODO: Check for FPS and "enabled" values.
		return;
	}

	// Check for expired messages.
	d->checkForExpiredMessages();

	if (!d->dirty) {
		// OSD is not dirty.
		// Call the Display List.
		glCallList(d->displayList);
		return;
	}

	// OSD is dirty.
	// Create a new GL display list.
	glNewList(d->displayList, GL_COMPILE_AND_EXECUTE);

	// Set pixel matrices.
	// Assuming 320x240 for 1x text rendering.
	// TODO: Use a larger matrix when rendering to a larger screen?
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
	glBindTexture(GL_TEXTURE_2D, d->texOsd);

	// Enable vertex and texture coordinate arrays.
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// TODO: Adjust for visible texture size.
	int y = (240 - d->chrH);
	// TODO: Message Enable, Message Color.

	// TODO: Switch from vector to list?
	// TODO: Move to OsdGLPrivate?
	for (int i = (int)d->osdList.size() - 1; i >= 0; i--) {
		OsdGLPrivate::OsdMessage *osdMsg = d->osdList[i];
		if (!osdMsg)
			continue;

		// NOTE: Message expiration is checked at the beginning of the function.

		// Message is now being displayed.
		osdMsg->hasDisplayed = true;

		// Next line.
		y -= d->chrH;

		// TODO: Make the drop shadow optional.
		glColor4f(0.0, 0.0, 0.0, 1.0);
		d->printLine(d->chrW+1, y+1, osdMsg->msg);
		d->setGLColor(d->msgColor);
		d->printLine(d->chrW, y, osdMsg->msg);
	}

	// Done with vertex and texture coordinate arrays.
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	// Reset the GL state.
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glDisable(GL_BLEND);			// Disable GL blending.
	glDisable(GL_TEXTURE_2D);		// Disable 2D textures.

	// Restore the matrices.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// Finished creating the GL display list.
	glEndList();
	d->dirty = false;
}

/**
 * Add a message to the OSD queue.
 * @param duration Duration for the message to appear, in milliseconds.
 * @param msg Message. (UTF-8)
 * TODO: printf() function.
 */
void OsdGL::print(unsigned int duration, const utf8_str *msg)
{
	// If the OSD is currently idle, reset the timer.
	// TODO: FPS.
	if (d->osdList.empty()) {
		// OSD is empty.
		d->timer.resetBase();
	}

	OsdGLPrivate::OsdMessage *osdMsg = new OsdGLPrivate::OsdMessage();

	// TODO: Convert msg from UTF-8.
	osdMsg->msg = string(msg);
	osdMsg->duration = duration;
	osdMsg->hasDisplayed = false;
	osdMsg->endTime = d->timer.getTime() + (duration * 1000);
	d->osdList.push_back(osdMsg);

	// OSD is dirty.
	d->dirty = true;
}

/** Properties. **/

bool OsdGL::isFpsEnabled(void) const
{
	return d->fpsEnabled;
}

void OsdGL::setFpsEnabled(bool fpsEnabled)
{
	if (d->fpsEnabled == fpsEnabled)
		return;

	d->fpsEnabled = fpsEnabled;
	// TODO: Force dirty...
}

bool OsdGL::isMsgEnabled(void) const
{
	return d->msgEnabled;
}

void OsdGL::setMsgEnabled(bool msgEnabled)
{
	if (d->msgEnabled == msgEnabled)
		return;

	d->msgEnabled = msgEnabled;
	if (!d->osdList.empty()) {
		// OSD messages are present.
		// Mark the OSD as dirty, since the messages
		// need to either be drawn if msgEnabled == true,
		// or hidden if msgEnabled == false.
		d->dirty = true;
	}
}

uint32_t OsdGL::fpsColor(void) const
{
	return d->fpsColor;
}

void OsdGL::setFpsColor(uint32_t fpsColor)
{
	if (d->fpsColor == fpsColor)
		return;

	d->fpsColor = fpsColor;
	// TODO: Force dirty...
}

uint32_t OsdGL::msgColor(void) const
{
	return d->msgColor;
}

void OsdGL::setMsgColor(uint32_t msgColor)
{
	if (d->msgColor == msgColor)
		return;

	d->msgColor = msgColor;
	if (d->msgEnabled && !d->osdList.empty()) {
		// OSD messages are present.
		// Mark the OSD as dirty, since the messages
		// need to be redrawn using the new color.
		d->dirty = true;
	}
}

}
