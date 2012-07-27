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

#include "lpc176x_usb_sie.h"
#include "lpc176x_usb_pvt.h"
#include "LPC17xx.h"

#define USB_SIE_CMD_PHASE_WR    (0x0100)
#define USB_SIE_CMD_PHASE_RD    (0x0200)
#define USB_SIE_CMD_PHASE_CMD   (0x0500)

#define USB_EPST_ST             (1 << 0)

static
inline
void
usb_sie_write_cmd
    (unsigned long  command
    )
{
    LPC_USB->USBDevIntClr   = USB_DI_CCEMPTY;
    LPC_USB->USBCmdCode     = (command << 16) | USB_SIE_CMD_PHASE_CMD;
    while (!(LPC_USB->USBDevIntSt & USB_DI_CCEMPTY));
}

static
inline
void
usb_sie_write_data
    (unsigned long  command
    ,unsigned long  data
    )
{
    (void)command;
    LPC_USB->USBDevIntClr   = USB_DI_CCEMPTY;
    LPC_USB->USBCmdCode     = (data << 16) | USB_SIE_CMD_PHASE_WR;
    while (!(LPC_USB->USBDevIntSt & USB_DI_CCEMPTY));
}

static
inline
unsigned
usb_sie_read_data
    (unsigned long  command
    )
{
    LPC_USB->USBDevIntClr   = USB_DI_CCEMPTY | USB_DI_CDFULL;
    LPC_USB->USBCmdCode     = (command << 16) | USB_SIE_CMD_PHASE_RD;
    while (!(LPC_USB->USBDevIntSt & USB_DI_CDFULL));
    return LPC_USB->USBCmdData & 0xff;
}

void
usb_sie_validate_buffer
    (void
    )
{
    usb_sie_write_cmd(0xfa);
}

void
usb_sie_clear_buffer
    (void
    )
{
    usb_sie_write_cmd(0xf2);
}

void
usb_sie_set_address
    (unsigned address
    )
{
    usb_sie_write_cmd(0xd0);
    usb_sie_write_data(0xd0, address | 0x80);
}

void
usb_sie_stall_endpoint
    (unsigned       phyiscal_endpoint
    )
{
    usb_sie_write_cmd(phyiscal_endpoint);
    usb_sie_write_data(phyiscal_endpoint, USB_EPST_ST);
}

void
usb_sie_select_endpoint
    (unsigned       phyiscal_endpoint
    )
{
    usb_sie_write_cmd(phyiscal_endpoint);
}

unsigned
usb_sie_get_device_status
    (void
    )
{
    LPC_USB->USBDevIntClr = USB_DI_DEV_STAT;
    usb_sie_write_cmd(0xfe);
    return usb_sie_read_data(0xfe);
}

void
usb_sie_set_device_status
    (unsigned       flags
    )
{
    usb_sie_write_cmd(0xfe);
    usb_sie_write_data(0xfe, flags & 0x1f);
}

void
usb_sie_configure_device
    (int            configured
    )
{
    usb_sie_write_cmd(0xd8);
    usb_sie_write_data(0xd8, (configured) ? 1 : 0);
}

void
usb_sie_set_mode
    (unsigned       mode
    )
{
    usb_sie_write_cmd(0xf3);
    usb_sie_write_data(0xf3, mode);
}

void
usb_sie_enable_endpoint
    (unsigned       physical_endpoint
    )
{
    const unsigned cmd = 0x40 | (0x1f & physical_endpoint);
    usb_sie_write_cmd(cmd);
    usb_sie_write_data(cmd, 0);
}

unsigned
usb_sie_get_frame_number
    (void
    )
{
    unsigned l;
    usb_sie_write_cmd(0xf5);
    l = usb_sie_read_data(0xf5);
    return l | ((usb_sie_read_data(0xf5) & 0x7) << 8);
}
