/***************************************************************************
 * libgens: Gens Emulation Library.                                        *
 * M68K.hpp: Main 68000 CPU wrapper class.                                 *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville.                      *
 * Copyright (c) 2003-2004 by Stéphane Akhoun.                             *
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

#include <libgens/config.libgens.h>

#include "M68K.hpp"
#include "M68K_Mem.hpp"

#include "macros/common.h"
#include "Cartridge/RomCartridgeMD.hpp"

// C includes. (C++ namespace)
#include <cstring>

namespace LibGens {

m68ki_cpu_core M68K::ms_Context;
int M68K::m_cycleCnt;
int M68K::m_intVectors[8];

// Last system ID.
M68K::SysID M68K::ms_LastSysID = SYSID_NONE;


// C wrapper functions for Starscream.
#ifdef __cplusplus
extern "C" {
#endif

static unsigned int Gens_M68K_RB(void *param, unsigned int address)
{
	return LibGens::M68K_Mem::M68K_RB(address);
}
static unsigned int Gens_M68K_RW(void *param, unsigned int address)
{
	return LibGens::M68K_Mem::M68K_RW(address);
}
static void Gens_M68K_WB(void *param, unsigned int address, unsigned int data)
{
	LibGens::M68K_Mem::M68K_WB(address, data);
}
static void Gens_M68K_WW(void *param, unsigned int address, unsigned int data)
{
	LibGens::M68K_Mem::M68K_WW(address, data);
}

uint8_t VDP_Int_Ack(void);

#ifdef __cplusplus
}
#endif

/**
 * Reset handler.
 * TODO: What does this function do?
 */
void M68K::M68K_Reset_Handler(m68ki_cpu_core *cpu)
{
	//Init_Memory_M68K(GENESIS);
}

int M68K::M68K_Int_Ack(m68ki_cpu_core *cpu, int int_level)
{
	if ( (int_level == 4) || (int_level == 6) )
		VDP_Int_Ack();

	m68k_set_irq(cpu, int_level, RESET_LINE);
	return m_intVectors[int_level];
}

static unsigned int dummy_read(void *param, unsigned int address)
{
	(void) param;
	(void) address;
	return ~0;
}

static void dummy_write(void *param, unsigned int address, unsigned int data)
{
	(void) param;
	(void) address;
	(void) data;
}


void M68K::SetFetch(unsigned low_addr, unsigned high_addr, void *base)
{
	unsigned i;

	base -= (low_addr & 0xFFFF);
	for ( i = (low_addr >> 16); i <= (high_addr >> 16); i ++ )
	{
		if ( i >= 256 )
			return;
		ms_Context.memory_map[i].base = (unsigned char *)base;
		/*ms_Context.memory_map[i].read8 = NULL;
		ms_Context.memory_map[i].read16 = NULL;
		ms_Context.memory_map[i].write8 = NULL;
		ms_Context.memory_map[i].write16 = NULL;*/
		base += 0x10000;
	}
}

void M68K::SetMemReadFunc(unsigned low_addr, unsigned high_addr,
	unsigned int (*read8)(void *param, unsigned int address),
	unsigned int (*read16)(void *param, unsigned int address))
{
	int i;
	for ( i = (low_addr >> 16); i <= (high_addr >> 16); i ++ )
	{
		//ms_Context.memory_map[i].param = this;
		ms_Context.memory_map[i].read8 = read8;
		ms_Context.memory_map[i].read16 = read16;
	}
}

void M68K::SetMemWriteFunc(unsigned low_addr, unsigned high_addr,
	void (*write8)(void *param, unsigned int address, unsigned int data),
	void (*write16)(void *param, unsigned int address, unsigned int data))
{
	int i;
	for ( i = (low_addr >> 16); i <= (high_addr >> 16); i ++ )
	{
		//ms_Context.memory_map[i].param = this;
		ms_Context.memory_map[i].write8 = write8;
		ms_Context.memory_map[i].write16 = write16;
	}
}

/**
 * Initialize the M68K CPU emulator.
 */
void M68K::Init(void)
{
	// Clear the 68000 context.
	memset( &ms_Context, 0, sizeof(ms_Context) );
	m68k_init(&ms_Context);

	// Initialize the memory handlers.
	SetMemReadFunc(0x000000, 0xFEFFFF, Gens_M68K_RB, Gens_M68K_RW);
	SetMemWriteFunc(0x000000, 0xFEFFFF, Gens_M68K_WB, Gens_M68K_WW);
	SetFetch(0xFF0000, 0xFFFFFF, Ram_68k.u8);
	
	ms_Context.reset_instr_callback = M68K_Reset_Handler;
	
	ms_Context.int_ack_callback = M68K_Int_Ack;

	//m68k_pulse_reset(&ms_Context);
}

/**
 * Shut down the M68K CPU emulator.
 */
void M68K::End(void)
{
	// TODO
}

/**
 * Initialize a specific system for the M68K CPU emulator.
 * @param system System ID.
 */
void M68K::InitSys(SysID system)
{
	// TODO: This is not 64-bit clean!
	ms_LastSysID = system;

	// Clear M68K RAM.
	memset(Ram_68k.u8, 0x00, sizeof(Ram_68k.u8));

	// Initialize the M68K memory handlers.
	M68K_Mem::InitSys(system);

	// Initialize M68K RAM handlers.
	for (int i = 0; i < 32; i++) {
		uint32_t ram_addr = (0xE00000 | (i << 16));
		SetFetch(ram_addr, ram_addr | 0xFFFF, Ram_68k.u8);
	}

	// Update the system-specific banking setup.
	UpdateSysBanking();

	// Reset the M68K CPU.
	Reset();
}

/**
 * Shut down M68K emulation.
 */
void M68K::EndSys(void)
{
	for (int i = 0; i < 256; i++) {
		ms_Context.memory_map[i].read8 = dummy_read;
		ms_Context.memory_map[i].read16 = dummy_read;
		ms_Context.memory_map[i].write8 = dummy_write;
		ms_Context.memory_map[i].write16 = dummy_write;
	}
	
}

/**
 * Update system-specific memory banking.
 * Uses the last system initialized via InitSys().
 */
void M68K::UpdateSysBanking(void)
{
	// Start at M68K_Fetch[0x20].
	int cur_fetch = 0x20;
	switch (ms_LastSysID) {
		case SYSID_MD:
		case SYSID_PICO:
			// Sega Genesis / Mega Drive.
			// Also Pico. (This only adds cartridge ROM.)
			cur_fetch += M68K_Mem::UpdateSysBanking(10);
			break;

		case SYSID_MCD:
			// Sega CD.
			// TODO
#if 0
			Bank_M68K = 0;
			
			// Set up Word RAM. (Entry 33)
			MS68K_Set_Word_Ram();
			
			// Set up Program RAM. (Entry 34)
			M68K_Fetch[34].lowaddr = 0x020000;
			M68K_Fetch[34].highaddr = 0x03FFFF;
			M68K_Set_Prg_Ram();
			
			// Terminate the list.
			M68K_Fetch[35].lowaddr = -1;
			M68K_Fetch[35].highaddr = -1;
			M68K_Fetch[35].offset = (unsigned int)NULL;
#endif
			break;
		
		case SYSID_32X:
			// Sega 32X.
			// TODO
#if 0
			Bank_SH2 = 0;
			
			// Nothing else is required. Terminate the list.
			M68K_Fetch[33].lowaddr  = -1;
			M68K_Fetch[33].highaddr = -1;
			M68K_Fetch[33].offset   = (unsigned int)NULL;
#endif
			break;
		
		default:
			break;
	}

	// FIXME: Make sure Starscream's internal program counter
	// is updated to reflect the updated M68K_Fetch[].
}

/** ZOMG savestate functions. **/

/**
 * ZomgSaveReg(): Save the M68K registers.
 * @param state Zomg_M68KRegSave_t struct to save to.
 */
void M68K::ZomgSaveReg(Zomg_M68KRegSave_t *state)
{
	// NOTE: Byteswapping is done in libzomg.
	int i;
	
	// Save the main registers.
	for (i = 0; i < 8; i++)
		state->dreg[i] = m68k_get_reg(&ms_Context, (m68k_register_t)(M68K_REG_D0 + i));
	for (i = 0; i < 7; i++)
		state->areg[i] = m68k_get_reg(&ms_Context, (m68k_register_t)(M68K_REG_A0 + i));
	
	// Save the stack pointers.
	state->ssp = ms_Context.s_flag ? ms_Context.dar[15] : ms_Context.sp[0];
	state->usp = m68k_get_reg(&ms_Context, M68K_REG_USP);

	// Other registers.
	state->pc = m68k_get_reg(&ms_Context, M68K_REG_PC);
	state->sr = m68k_get_reg(&ms_Context, M68K_REG_SR);

	// Reserved fields.
	state->reserved1 = 0;
	state->reserved2 = 0;
}


/**
 * Restore the M68K registers.
 * @param state Zomg_M68KRegSave_t struct to restore from.
 */
void M68K::ZomgRestoreReg(const Zomg_M68KRegSave_t *state)
{
	int i;
	
	// Load the main registers.
	for (i = 0; i < 8; i++)
		m68k_set_reg(&ms_Context, (m68k_register_t)(M68K_REG_D0 + i), state->dreg[i]);
	for (i = 0; i < 7; i++)
		m68k_set_reg(&ms_Context, (m68k_register_t)(M68K_REG_A0 + i), state->areg[i]);

	// Other registers.
	m68k_set_reg(&ms_Context, M68K_REG_PC, state->pc);
	m68k_set_reg(&ms_Context, M68K_REG_SR, state->sr);

	// Load the stack pointers.
	m68k_set_reg(&ms_Context, M68K_REG_USP, state->usp);
	if ( ms_Context.s_flag )
		ms_Context.dar[15] = state->ssp;
	else
		ms_Context.sp[0] = state->ssp;
}

}
