/***************************************************************************
 * libcompat: Compatibility library.                                       *
 * byteorder.h: System byte order header.                                  *
 *                                                                         *
 * Copyright (c) 2011-2015 by David Korth.                                 *
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

#ifndef __LIBCOMPAT_BYTEORDER_H__
#define __LIBCOMPAT_BYTEORDER_H__

/**
 * Check the system byte order.
 *
 * We're doing the test here instead of in CMake because:
 * 1. CMake doesn't support testing multiple architectures
 *    at the same time. (Mac OS X universal binary)
 * 2. CMake can't test the byte order when cross-compiling to
 *    a different CPU.
 *
 * Based on SDL_endian.h from SDL 1.2 and 2.0.
 * Other references:
 * - http://nadeausoftware.com/articles/2012/02/c_c_tip_how_detect_processor_type_using_compiler_predefined_macros
 * - http://sourceforge.net/p/predef/wiki/Architectures/
 */

#define SYS_LIL_ENDIAN 1234
#define SYS_BIG_ENDIAN 4321
#define SYS_PDP_ENDIAN 3412

#if defined(__hppa__) || defined(__HPPA__) || \
    defined(__m68k__) || defined(__MC68K__) || defined(_M_M68K) || \
    defined(mc68000) || defined(M68000) || \
    ((defined(__mips) || defined(__mips__) || defined(__mips) || defined(__MIPS__)) && \
     (defined(__mipseb) || defined(__mipseb__) || defined(__MIPSEB) || defined(__MIPSEB__))) || \
    defined(__powerpc__) || defined(__POWERPC__) || \
    defined(__ppc__) || defined(__PPC__) || defined(_M_PPC) || \
    defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || \
    defined(__sparc) || defined(__sparc__)

/* System is big-endian. */
#define SYS_BYTEORDER SYS_BIG_ENDIAN
#define SYS_IS_BIG_ENDIAN 1
#undef SYS_IS_LIL_ENDIAN
#undef SYS_IS_PDP_ENDIAN

#else

/* System is little-endian. */
#define SYS_BYTEORDER SYS_LIL_ENDIAN
#define SYS_IS_LIL_ENDIAN 1
#undef SYS_IS_BIG_ENDIAN
#undef SYS_IS_PDP_ENDIAN

#endif

#endif /* __LIBCOMPAT_BYTEORDER_H__ */
