/******************************************************************************
 * libgens: Gens Emulation Library.                                           *
 * lg_main.cpp: Main emulation code.                                          *
 *                                                                            *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville.                         *
 * Copyright (c) 2003-2004 by Stéphane Akhoun.                                *
 * Copyright (c) 2008-2011 by David Korth.                                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 2 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software                *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA *
 ******************************************************************************/

#include <config.h>

#include "lg_main.hpp"
#include "macros/git.h"
#include "Util/cpuflags.h"
#include "Util/Timing.hpp"

// CPU emulation code.
#include "cpu/M68K.hpp"
#include "cpu/M68K_Mem.hpp"
#include "cpu/Z80.hpp"
#include "cpu/Z80_MD_Mem.hpp"

// System emulation code.
#include "Vdp/Vdp.hpp"

// Input Device Manager.
#include "GensInput/DevManager.hpp"

// Sound Manager.
#include "sound/SoundMgr.hpp"

// C includes.
#include <stdio.h>

// C++ includes.
#include <string>
using std::string;

// Win32 Unicode Translation Layer.
#ifdef _WIN32
#include "Win32/W32U_mini.h"
#endif


namespace LibGens
{

static bool ms_IsInit = false;

// libgens version. (TODO: Use MDP version macros.)
const unsigned int version = 0x00000000U;
const char *version_desc = "Development Build";	// ASCII

// Version Control System revision. (ASCII) (May be NULL.)
#ifdef GENS_GIT_VERSION
const char *version_vcs = GENS_GIT_VERSION;
#else
const char *version_vcs = NULL;
#endif

/**
 * IsRunning(): Determines if LibGens is running.
 * @return True if the LibGens thread is running; false otherwise.
 */
bool IsRunning(void)
{
	return ms_IsInit;
}


/**
 * Init(): Initialize LibGens.
 * @return 0 on success; non-zero on error.
 */
int Init(void)
{
	// TODO: Reference counting?
	if (ms_IsInit)
		return 0;
	
	// Print the Gens/GS startup message.
	fprintf(stderr, "Gens/GS II");
	if (version_desc)
		fprintf(stderr, " (%s)", version_desc);
	fputc('\n', stderr);
	
#if !defined(GENS_ENABLE_EMULATION)
	fprintf(stderr, "[NO-EMULATION BUILD; CPU emulation disabled.]\n");
#endif
	
	// VCS version.
	if (version_vcs)
		fprintf(stderr, "(%s)\n", version_vcs);
	
	fprintf(stderr, "\n"
		"Copyright (c) 1999-2002 by Stéphane Dallongeville.\n"
		"Copyright (c) 2003-2004 by Stéphane Akhoun.\n"
		"Copyright (c) 2008-2011 by David Korth.\n"
		"\n");
	
	// GNU GPLv2 notice.
	// References:
	// * http://www.gnu.org/licenses/gpl-howto.html
	// * http://web.archive.org/web/20070121002445/http://www.gnu.org/licenses/gpl-howto.html
	fprintf(stderr,
		"This program is free software; you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation; either version 2 of the License, or\n"
		"(at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program; if not, write to the Free Software\n"
		"Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA\n"
		"\n");
	
	// TODO: Clear LibGens variables.
	
	// Detect CPU flags.
	LibGens_GetCPUFlags();
	
	// Win32 Unicode Translation Layer.
#ifdef _WIN32
	W32U_Init();
#endif
	
	// Initialize LibGens subsystems.
	Timing::Init();
	M68K::Init();
	M68K_Mem::Init();
	Z80::Init();
	Z80_MD_Mem::Init();
	
	// MD VDP. (TODO: Make this non-static.)
	Vdp::Init();
	
	DevManager::Init();
	SoundMgr::Init();
	
	ms_IsInit = true;
	return 0;
}


/**
 * End(): Stop LibGens.
 * TODO
 * @return 0 on success; non-zero on error.
 */
int End(void)
{
	// TODO
	if (!ms_IsInit)
		return 1;
	
	// TODO: Add CpuFlags::End() or something similar.
	CPU_Flags = 0;
	
	// Shut down LibGens subsystems.
	Timing::End();
	M68K::End();
	M68K_Mem::End();
	Z80::End();
	Z80_MD_Mem::End();
	
	// MD VDP. (TODO: Make this non-static.)
	Vdp::End();
	
	DevManager::End();
	SoundMgr::End();
	
	// Win32 Unicode Translation Layer.
#ifdef _WIN32
	W32U_End();
#endif
	
	ms_IsInit = false;
	return 0;
}

}
