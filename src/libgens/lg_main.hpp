/***************************************************************************
 * libgens: Gens Emulation Library.                                        *
 * lg_main.hpp: Main emulation code.                                       *
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

#ifndef __LIBGENS_LG_MAIN_HPP__
#define __LIBGENS_LG_MAIN_HPP__

// Needed for NULL.
#include <string.h>

#include "MtQueue.hpp"

namespace LibGens
{

bool IsRunning(void);

int Init(const void *wid, const char *sWinTitle = NULL);
int End(void);

int LgThread(void *param);

// UI to SDL queue.
extern MtQueue *qToSDL;

}

#endif /* __LIBGENS_LG_MAIN_HPP__ */
