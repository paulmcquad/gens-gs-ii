/********************************************************************************/
/*                                                                              */
/* CZ80: byteorder definitions                                                  */
/* C Z80 emulator version 0.91 	                                                */
/* Copyright 2004 Stephane Dallongeville                                        */
/*                                                                              */
/* Copied from libcompat/byteorder.h from Gens/GS II.                           */
/*                                                                              */
/********************************************************************************/

#ifndef _CZ80_BYTEORDER_H_
#define _CZ80_BYTEORDER_H_

#include "libcompat/byteorder.h"

#if SYS_IS_LIL_ENDIAN

/* System is little-endian. */
// TODO: Set this somewhere.
#define CZ80_LITTLE_ENDIAN 1

#else

/* System is big-endian. */
#define CZ80_LITTLE_ENDIAN 0

#endif

#endif /* _CZ80_BYTEORDER_H_ */
