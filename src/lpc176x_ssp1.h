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

#ifndef LPC176X_SSP1_H_
#define LPC176X_SSP1_H_

#define SSP_PROTOCOL_TI                 (0)
#define SSP_PROTOCOL_MICROWIRE          (1)
#define SSP_PROTOCOL_SPI                (2)

#define SSP_MODE_SLAVE                  (0)
#define SSP_MODE_SLAVE_NO_OUTPUT        (1)
#define SSP_MODE_MASTER                 (2)

/* Clock for SPI is high in idle mode (as opposed to low) */
#define SSP_FLAG_SPI_CLK_IDLE_HIGH      (0x0001)

/* Data for SPI is captured on clock falling edges (rather than rising edges) */
#define SSP_FLAG_SPI_CLK_FALLING_EDGE   (0x0002)

/* Serial input taken directly from serial output */
#define SSP_FLAG_LOOPBACK_MODE          (0x0004)

struct ssp_config_s
{
    unsigned long   baud_rate;
    unsigned        bits_per_frame;
    unsigned        protocol;
    unsigned        mode;
    unsigned        flags;
};


void ssp1_setup(unsigned long cclk, const struct ssp_config_s *config);


#endif /* LPC176X_SSP1_H_ */
