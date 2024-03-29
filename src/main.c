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

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include "usb_midi.h"
#include <cr_section_macros.h>
#include <NXP/crp.h>
#include "conbus.h"

__CRP const unsigned int CRP_WORD = CRP_NO_CRP ;

static unsigned char conbus_data[1024];


int main(void)
{
    struct conbus_config_s cfg;
    cfg.nb_inputs_div_8 = 1;
    cfg.nb_outputs_div_8 = 1;
    conbus_init(&cfg, conbus_data);
    usb_midi_setup(12000000UL);
    for (;;)
    {
    }
	return 0;
}
