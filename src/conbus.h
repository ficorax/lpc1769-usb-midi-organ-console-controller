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

#ifndef CONBUS_H_
#define CONBUS_H_

struct conbus_config_s
{
    unsigned    nb_inputs_div_8;  /* Every input requires 1 byte */
    unsigned    nb_outputs_div_8; /* Every output requires 1 bit */
};

void conbus_init(const struct conbus_config_s *config, unsigned char *memory);



#endif /* CONBUS_H_ */
