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

#ifndef USB_SIE_H_
#define USB_SIE_H_

void        usb_sie_validate_buffer(void);
void        usb_sie_clear_buffer(void);
void        usb_sie_set_address(unsigned address);
void        usb_sie_stall_endpoint(unsigned phyiscal_endpoint);
void        usb_sie_select_endpoint(unsigned phyiscal_endpoint);
unsigned    usb_sie_get_device_status(void);
void        usb_sie_set_device_status(unsigned flags);
void        usb_sie_configure_device(int configured);
void        usb_sie_set_mode(unsigned mode);
void        usb_sie_enable_endpoint(unsigned physical_endpoint);
unsigned    usb_sie_get_frame_number(void);

#endif /* LPC176X_USB_SIE_H_ */
