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

#include "conbus.h"
#include "LPC17xx.h"
#include "lpc176x_ssp1.h"

static unsigned char *input_memory;
static unsigned char *debounce_mempos;
static unsigned char *output_memory;
static unsigned       write_pos;
static unsigned       read_pos;
static unsigned       start_reading_input;
static unsigned       start_writing_output;
static unsigned       bus_length;

#define DEBOUNCE_TICKS (10)

void conbus_init(const struct conbus_config_s *config, unsigned char *memory)
{
    /* Setup SPI */
    {
        struct ssp_config_s ssp_cfg;
        ssp_cfg.baud_rate      = 60000;
        ssp_cfg.bits_per_frame = 8;
        ssp_cfg.flags          = 0;
        ssp_cfg.mode           = SSP_MODE_MASTER;
        ssp_cfg.protocol       = SSP_PROTOCOL_SPI;
        ssp1_setup(100000000UL, &ssp_cfg);
    }

    /* Setup conbus */
    output_memory  = memory;
    input_memory   = memory + config->nb_outputs_div_8;
    if (config->nb_inputs_div_8 > config->nb_outputs_div_8)
    {
        bus_length           = config->nb_inputs_div_8;
        start_writing_output = config->nb_inputs_div_8 - config->nb_outputs_div_8;
        start_reading_input  = 0;
    }
    else
    {
        bus_length           = config->nb_outputs_div_8;
        start_writing_output = 0;
        start_reading_input  = config->nb_outputs_div_8 - config->nb_inputs_div_8;
    }

    /* Setup parallel load / output latch pin */
    LPC_GPIO2->FIODIR = 1 << 13;

    /* Temp indicator */
    LPC_GPIO0->FIODIR |= 1 << 2;


    /* setup timer for bus reads */
    LPC_TIM0->CTCR  = 0;
    LPC_TIM0->MCR   = 0x3;
    LPC_TIM0->PR    = 24999;
    LPC_TIM0->MR0   = 5; /* Milliseconds */
    LPC_TIM0->TCR   = 1;


    NVIC_SetPriority(SSP1_IRQn, 9);
    NVIC_EnableIRQ(SSP1_IRQn);
    NVIC_SetPriority(TIMER0_IRQn, 8);
    NVIC_EnableIRQ(TIMER0_IRQn);

}

void SSP1_IRQHandler(void)
{
    LPC_SSP1->ICR = (1 << 1);

    while ((LPC_SSP1->SR & (1 << 2)) && (read_pos < bus_length))
    {
        if (read_pos < start_reading_input)
        {
            (void)LPC_SSP1->DR;
        }
        else
        {
            unsigned new_ip_state = LPC_SSP1->DR;
            unsigned old_ip_state = input_memory[read_pos - start_reading_input];
            unsigned i;
            for (i = 0; i < 8; i++)
            {
                unsigned debouncer = debounce_mempos[i];
                if (debouncer & 0x7f) debouncer--;
                if ((new_ip_state >> i) & 1)
                {
                    /* Key is down */
                    if (!(debouncer & 0x80))
                    {
                        /* Debouncer is up... */
                        debouncer = 0x80 | DEBOUNCE_TICKS;
                        old_ip_state |= (1 << i);
                    }
                }
                else
                {
                    /* Key is up */
                    if (debouncer & 0x80)
                    {
                        /* Debouncer is down */
                        debouncer = DEBOUNCE_TICKS;
                    }
                    else if (debouncer == 0)
                    {
                        /* Key has been up for debounce ticks - update state */
                        old_ip_state &= ~(1 << i);
                    }
                }
                input_memory[read_pos - start_reading_input] = old_ip_state;
                output_memory[0] = old_ip_state;
                debounce_mempos[i] = debouncer;
            }
            debounce_mempos += 8;
        }
        read_pos++;
    }
    if (read_pos >= bus_length)
    {
        /* Disable all read interrupts if all data read */
        LPC_SSP1->IMSC &= ~((1 << 1) | (1 << 2));
        LPC_GPIO2->FIOCLR = 1 << 13;
    }

    while ((LPC_SSP1->SR & (1 << 1)) && (write_pos < bus_length))
    {
        if (write_pos < start_writing_output)
        {
            LPC_SSP1->DR = 0;
        }
        else
        {
            LPC_SSP1->DR = output_memory[write_pos - start_writing_output];
        }
        write_pos++;
    }
    if (write_pos >= bus_length)
    {
        /* Disable transmit interrupt if all data written */
        LPC_SSP1->IMSC &= ~(1 << 3);
    }
}

void TIMER0_IRQHandler(void)
{
    LPC_TIM0->IR = 1;
    if (!LPC_SSP1->IMSC)
    {
        LPC_GPIO2->FIOSET = 1 << 13;
        write_pos       = 0;
        read_pos        = 0;
        debounce_mempos = input_memory + (bus_length - start_reading_input);
        LPC_SSP1->IMSC  = (1 << 3) | (1 << 2) | (1 << 1);
    }
#if 1
    if (LPC_GPIO0->FIOPIN & (1 << 2))
        LPC_GPIO0->FIOCLR = 1 << 2;
    else
        LPC_GPIO0->FIOSET = 1 << 2;
#endif
}



