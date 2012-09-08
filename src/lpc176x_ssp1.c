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

#include "lpc176x_ssp1.h"
#include <LPC17xx.h>
#include "debughlprs.h"


#define PCONP_SSP1             (1ul << 10)

static
void
ssp1_set_control_registers
    (unsigned protocol
    ,unsigned serial_clock_rate
    ,unsigned bits_per_frame
    ,unsigned idle_high_clk
    ,unsigned falling_edge_capture
    ,unsigned is_loopback
    ,unsigned mode
    )
{
    unsigned long cr0;
    unsigned long cr1;
    ASSERT((bits_per_frame >= 4) && (bits_per_frame <= 16));
    ASSERT(serial_clock_rate <= 255);
    cr0 = ((serial_clock_rate & 0xff) << 8) | ((bits_per_frame - 1) & 0xf);
    switch (protocol)
    {
    case SSP_PROTOCOL_TI:
        cr0 |= 0x10;
        break;
    case SSP_PROTOCOL_MICROWIRE:
        cr0 |= 0x20;
        break;
    default:
        ASSERT(config->protocol == SSP_PROTOCOL_SPI);
        if (idle_high_clk)
        {
            cr0 |= 0x40;
            if (!falling_edge_capture)
            {
                cr0 |= 0x80;
            }
        }
        else
        {
            if (falling_edge_capture)
            {
                cr0 |= 0x80;
            }
        }
    }
    switch (mode)
    {
    case SSP_MODE_SLAVE:
        cr1 = 0x04;
        break;
    case SSP_MODE_SLAVE_NO_OUTPUT:
        cr1 = 0x0c;
        break;
    default:
        ASSERT(mode == SSP_MODE_MASTER);
        cr1 = 0x00;
    }
    if (is_loopback)
    {
        cr1 |= 0x01;
    }
    LPC_SSP1->CR0 = cr0;
    LPC_SSP1->CR1 = cr1;
    LPC_SSP1->CR1 = cr1 | 0x02;
}

static
void
ssp_find_best_divisors
    (unsigned long  target_2
    ,unsigned      *serial_clock_rate /* Will be between 0 and 255 (represents 1 to 256) */
    ,unsigned      *prescale_divisor  /* Will be an even number between 2 and 254 */
    )
{
    unsigned long cpsdvsr_2         = 1;
    unsigned long scr               = target_2 / cpsdvsr_2;
    unsigned long best_error        = (scr > 256) ? target_2 : (target_2 - (cpsdvsr_2 * scr));
    unsigned long best_scr          = (scr > 256) ? 256 : scr;
    unsigned long best_cpsdvsr_2    = cpsdvsr_2;

    ASSERT(tgt_div <= 256 * 127);

    while ((best_error > 0) && (cpsdvsr_2 < 127))
    {
        unsigned long error;
        cpsdvsr_2   = cpsdvsr_2 + 1;
        scr         = target_2 / cpsdvsr_2;
        error       = target_2 - (cpsdvsr_2 * scr);
        if ((error < best_error) && (scr <= 256))
        {
            best_error      = error;
            best_scr        = scr;
            best_cpsdvsr_2  = cpsdvsr_2;
        }
    }

    *serial_clock_rate  = best_scr - 1;
    *prescale_divisor   = 2 * best_cpsdvsr_2;
}

struct ssp_clock_config_s
{
    unsigned serial_clock_rate;
    unsigned clock_prescale_divisor;
    unsigned prescale;
};

static
struct ssp_clock_config_s
ssp_get_clock_config
    (unsigned long cclk
    ,unsigned long baud_rate
    ,int           is_master
    )
{
    struct ssp_clock_config_s cfg;
    cfg.serial_clock_rate       = 0;
    cfg.clock_prescale_divisor  = 2;
    if (is_master)
    {
        /* Easier to do search when target divisor is given divided by two
         * due to clock prescale divisor needing to be a multiple of two. */
        unsigned long tgt_2 = (cclk + baud_rate) / (baud_rate * 2);
        /* Find the best prescale divisor */
        for (cfg.prescale = 0
            ;   (   (   ((tgt_2 & 1) == 0)
                    ||  (tgt_2 > 256 * 127)
                    )
                &&  (cfg.prescale < 3)
                )
            ;cfg.prescale++, tgt_2 >>= 1
            );
        /* Find the clock divisors */
        ssp_find_best_divisors
            (tgt_2
            ,&cfg.serial_clock_rate
            ,&cfg.clock_prescale_divisor
            );
    }
    else
    {
        const unsigned long ssp_clk_target = baud_rate * 12;
        /* Find the best prescale divisor */
        for (cfg.prescale = 3
            ;(cfg.prescale >= 1) && ((cclk >> cfg.prescale) < ssp_clk_target)
            ;cfg.prescale--
            );
        /* The serial clock rate and clock prescale divisor are irrelevant in
         * slave mode. */
        ASSERT((cclk >> cfg.prescale) >= ssp_clk_target);
    }
    return cfg;
}

void ssp1_setup(unsigned long cclk, const struct ssp_config_s *config)
{
    static const unsigned long PRESCALER_MASK = 0xffcffffful;
    static const unsigned long PRESCALER_FLAGS[4] =
        {0x00100000ul /* PCLK = CCLK */
        ,0x00200000ul /* PCLK = CCLK / 2 */
        ,0x00000000ul /* PCLK = CCLK / 4 */
        ,0x00300000ul /* PCLK = CCLK / 8 */
        };

    struct ssp_clock_config_s clock_config =
        ssp_get_clock_config
            (cclk
            ,config->baud_rate
            ,config->mode == SSP_MODE_MASTER
            );


    LPC_SC->PCONP          |= PCONP_SSP1;
    LPC_PINCON->PINSEL0     = (LPC_PINCON->PINSEL0 & 0xfff00ffful) | 0x000aa000ul;
    LPC_SC->PCLKSEL0        = (LPC_SC->PCLKSEL0 & PRESCALER_MASK) | PRESCALER_FLAGS[clock_config.prescale];
    LPC_SSP1->CPSR          = clock_config.clock_prescale_divisor;
    ssp1_set_control_registers
        (config->protocol
        ,clock_config.serial_clock_rate
        ,config->bits_per_frame
        ,config->flags & SSP_FLAG_SPI_CLK_IDLE_HIGH
        ,config->flags & SSP_FLAG_SPI_CLK_FALLING_EDGE
        ,config->flags & SSP_FLAG_LOOPBACK_MODE
        ,config->mode
        );



}



