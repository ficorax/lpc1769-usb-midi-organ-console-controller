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

#ifndef USB_H_
#define USB_H_

#define USB_DESC_TYPE_DEVICE            (0x01)
#define USB_DESC_TYPE_CONFIGURATION     (0x02)
#define USB_DESC_TYPE_STRING            (0x03)
#define USB_DESC_TYPE_INTERFACE         (0x04)
#define USB_DESC_TYPE_CS_INTERFACE      (0x24)
#define USB_DESC_TYPE_ENDPOINT          (0x05)
#define USB_DESC_TYPE_CS_ENDPOINT       (0x25)
#define USB_DESC_TYPE_DEVICE_QUALIFIER  (0x06)
#define USB_DESC_TYPE_OTHER_SPEED_CFG   (0x07)
#define USB_DESC_TYPE_INTERFACE_POWER   (0x08)

struct usb_configuration_s
{
    /* Returns a complete device descriptor */
    const unsigned char*      dev_desc;
    /* Returns a complete configuration descriptor. This descriptor contains
     * all of the information which is used to configure endpoints etc. Ensure
     * that the endpoints are correct for the type they need to be for the
     * device (i.e. ep's 4 and 5 must be "bulk" - see lpc1769 documentation) */
    const unsigned char*    (*get_cfg_desc)(unsigned index);
    /* Returns a string descriptor or null if it does not exist. String ID 0
     * is a language table (see USB2.0 9.6.7) */
    const unsigned char*    (*get_string_desc)(unsigned string_id, unsigned lang_id);
    /* Function to call on a frame interrupt. */;
    void                    (*on_usb_frame)(unsigned frame_number);
    /* Function which is called for endpoint transfers. Note that the physical
     * endpoint parameter is not the same as the endpoint indicies supplied in
     * the configuration descriptor. physical_endpoints correspond to the
     * enpoints as listed in the device documentation. */
    void                    (*on_usb_endpoint)(unsigned physical_endpoint);
};

/* Setup and enable the USB device with the given descriptor */
void        usb_setup(unsigned long fosc, const struct usb_configuration_s *usb_config);
/* Returns non-zero if the device is configured */
int         usb_is_configured(void);
/* Write to the given physical endpoint. The return value is how much data was
 * successfully written (will not be more than the maximum endpoint buffer size
 * as set in the configuration descriptor and will always be less than 64
 * characters). */
unsigned    usb_write(unsigned physical_endpoint, const unsigned char *buffer, unsigned size);
/* Read from the given physical endpoint. Returns negative on error otherwise
 * the return value is the number of chars read into the buffer. */
int         usb_read(unsigned physical_endpoint, unsigned char *buffer, unsigned buffer_size);

#endif /* LPC176X_USB_H_ */
