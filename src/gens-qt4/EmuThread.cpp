/***************************************************************************
 * gens-qt4: Gens Qt4 UI.                                                  *
 * EmuThread.hpp: Emulation thread.                                        *
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

#include "EmuThread.hpp"

#include "libgens/MD/EmuMD.hpp"
#include "libgens/MD/VdpIo.hpp"
#include "libgens/MD/VdpPalette.hpp"

namespace GensQt4
{

EmuThread::EmuThread()
{
	m_stop = false;
}

EmuThread::~EmuThread()
{
}

void EmuThread::resume(void)
{
	m_mutex.lock();
	m_wait.wakeAll();
	m_mutex.unlock();
}

void EmuThread::stop(void)
{
	m_mutex.lock();
	m_stop = true;
	m_wait.wakeAll();
	m_mutex.unlock();
}

#include <stdio.h>
void EmuThread::run(void)
{
	// NOTE: LibGens initialization is done elsewhere.
	// The emulation thread doesn't initialize anything;
	// it merely runs what's already been initialized.
	
	// Run the emulation thread.
	m_mutex.lock();
	while (!m_stop)
	{
		// Run a frame of emulation.
		LibGens::EmuMD::Do_Frame();
		
		// Signal that the frame has been drawn.
		emit frameDone();
		
		// Wait for a resume command.
		m_wait.wait(&m_mutex);
	}
	m_mutex.unlock();
}

}
