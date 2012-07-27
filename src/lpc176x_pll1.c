/* LPC176x based USB MIDI Organ Console Controller
 * Copyright (C) 2012  Nick Appleton (http://www.appletonaudio.com/)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <LPC17xx.h>
#include "lpc176x_pll1.h"
#include "debughlprs.h"

#define PLL1_PLOCK      (1ul << 10)
#define PLL1_PLLC1_STAT (1ul << 9)
#define PLL1_PLLE1_STAT (1ul << 8)

/* FIXME: maybe this should be more flexible and take the desired output
 * frequency. Currently, this is always 48MHz for the USB controller. */
void pll1_setup(unsigned long fosc)
{
	unsigned long m = 48000000 / fosc;
	ASSERT((m > 1) && (m * fosc == 48000000));

	LPC_SC->PLL1CFG   = 0x20 | (m - 1);
	LPC_SC->PLL1FEED  = 0xaa;
	LPC_SC->PLL1FEED  = 0x55;

	LPC_SC->PLL1CON   = 0x01;
	LPC_SC->PLL1FEED  = 0xaa;
	LPC_SC->PLL1FEED  = 0x55;
	while (!(LPC_SC->PLL1STAT & PLL1_PLOCK));

	LPC_SC->PLL1CON   = 0x03;
	LPC_SC->PLL1FEED  = 0xaa;
	LPC_SC->PLL1FEED  = 0x55;
	while (!(LPC_SC->PLL1STAT & (PLL1_PLLC1_STAT | PLL1_PLLE1_STAT)));
}
