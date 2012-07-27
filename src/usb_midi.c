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

#include "lpc176x_usb.h"
#include "lpc176x_usb_sie.h"

/* MS Class-Specific Interface Descriptor Subtypes */
#define MS_IFACE_DESC_UNDEFINED     (0x00)
#define MS_IFACE_DESC_HEADER        (0x01)
#define MS_IFACE_DESC_MIDI_IN_JACK  (0x02)
#define MS_IFACE_DESC_MIDI_OUT_JACK (0x03)
#define MS_IFACE_DESC_ELEMENT       (0x04)

/* MS Class-Specific Endpoint Descriptor Subtypes */
#define MS_ENDP_DESC_UNDEFINED      (0x00)
#define MS_ENDP_DESC_GENERAL        (0x01)

/* MS MIDI IN and OUT Jack Types */
#define MS_MIDI_IO_JACK_UNDEFINED   (0x00)
#define MS_MIDI_IO_JACK_EMBEDDED    (0x01)
#define MS_MIDI_IO_JACK_EXTERNAL    (0x02)

/* 5.1) Control Selectors - Endpoint */
#define CS_UNDEFINED                (0x00)
#define CS_ASSOCIATION_CONTROL      (0x01)

static
const unsigned char midi_desc[] =
    {   18
    ,   0x01
    ,   0x10 /* usb 1.10 */
    ,   0x01 /* usb 1.10 */
    ,   0x00
    ,   0x00 /* unused */
    ,   0x00 /* unused */
    ,   0x08
    ,   0x00 /* ven id low */
    ,   0x00 /* ven id high */
    ,   0x00 /* prod id low */
    ,   0x00 /* prod id high */
    ,   0x01 /* dev rel code id low */
    ,   0x00 /* dev rel code id high */
    ,   0x01 /* mfg string id */
    ,   0x02 /* prod string id */
    ,   0x00 /* serial nb */
    ,   0x01 /* 1 cfg */
    };

static
const unsigned char midi_conf_desc[] =
    {   9
    ,   USB_DESC_TYPE_CONFIGURATION
    ,   101     /* size of the entire descriptor + endpoints */
    ,   0       /* msb of above */
    ,   2       /* nb of interfaces */
    ,   1       /* this is configuration id 1 */
    ,   0       /* unused */
    ,   0x40    /* self-powered */
    ,   0       /* no power requirements */
    /* b3.1) Interface 1 */
    ,   9
    ,   USB_DESC_TYPE_INTERFACE
    ,   0       /* index of this interface */
    ,   0
    ,   0       /* nb of endpoints */
    ,   1       /* AUDIO */
    ,   1       /* AUDIO_CONTROL */
    ,   0       /* not used */
    ,   0       /* not used */
    /* b3.2) class specific descriptor */
    ,   9
    ,   USB_DESC_TYPE_CS_INTERFACE
    ,   1
    ,   0x00
    ,   0x10
    ,   0x09
    ,   0x00
    ,   1
    ,   1
    /* b4.1) Midi streaming descriptor */
    ,   9
    ,   USB_DESC_TYPE_INTERFACE
    ,   1       /* index of this interface */
    ,   0
    ,   2       /* nb of endpoints */
    ,   1       /* AUDIO */
    ,   3       /* MIDISTREAMING */
    ,   0       /* not used */
    ,   0       /* not used */
    /* b4.2) class specific midi streaming */
    ,   7
    ,   USB_DESC_TYPE_CS_INTERFACE
    ,   MS_IFACE_DESC_HEADER
    ,   0x00
    ,   0x01
    ,   65      /* size */
    ,   0
    /* b4.3 midi in jack 1 */
    ,   6
    ,   USB_DESC_TYPE_CS_INTERFACE
    ,   MS_IFACE_DESC_MIDI_IN_JACK
    ,   MS_MIDI_IO_JACK_EMBEDDED
    ,   0x01    /* jack ID */
    ,   0x00    /* unused */
    /* midi in jack 2 */
    ,   6
    ,   USB_DESC_TYPE_CS_INTERFACE
    ,   MS_IFACE_DESC_MIDI_IN_JACK
    ,   MS_MIDI_IO_JACK_EXTERNAL
    ,   0x02    /* jack ID */
    ,   0       /* unused */
    /* b4.4) midi out jack */
    ,   9
    ,   USB_DESC_TYPE_CS_INTERFACE
    ,   MS_IFACE_DESC_MIDI_OUT_JACK
    ,   MS_MIDI_IO_JACK_EMBEDDED
    ,   0x03    /* jack ID */
    ,   0x01    /* input pins */
    ,   0x02    /* source id */
    ,   0x01    /* source pin */
    ,   0x00    /* unused */
    /* midi out jack */
    ,   9
    ,   USB_DESC_TYPE_CS_INTERFACE
    ,   MS_IFACE_DESC_MIDI_OUT_JACK
    ,   MS_MIDI_IO_JACK_EXTERNAL
    ,   0x04    /* jack ID */
    ,   0x01    /* input pins */
    ,   0x01    /* source id */
    ,   0x01    /* source pin */
    ,   0x00    /* unused */
    /* b5.1) std bulk out endpoint desc */
    ,   9
    ,   USB_DESC_TYPE_ENDPOINT
    ,   0x02    /* OUT endpoint 2 - bulk*/
    ,   2       /* bmAttributes - bulk */
    ,   64      /* 64 bytes per packet */
    ,   0x00
    ,   0       /* ignroed for bulk */
    ,   0       /* unused */
    ,   0       /* unused */
    /* b5.2) class specific ms bulk out endpoint desc */
    ,   5
    ,   USB_DESC_TYPE_CS_ENDPOINT
    ,   MS_ENDP_DESC_GENERAL
    ,   1       /* nb embedded midi's */
    ,   1       /* id of embedded midi */
    /* b6.1) std bulk in endpoint desc */
    ,   9
    ,   USB_DESC_TYPE_ENDPOINT
    ,   0x82    /* IN endpoint 2 */
    ,   2       /* bulk */
    ,   64      /* 64 bytes per packet */
    ,   0x00
    ,   0       /* ignored for bulk */
    ,   0       /* unused */
    ,   0       /* unused */
    /* b6.2) class specific ms bulk in endpoint desc */
    ,   5
    ,   USB_DESC_TYPE_CS_ENDPOINT
    ,   MS_ENDP_DESC_GENERAL
    ,   1       /* nb embedded midi's */
    ,   3       /* id of embedded midi */
    };

static
const unsigned char midi_mfg_str[] =
    {   28
    ,   USB_DESC_TYPE_STRING
    ,   'N', 0
    ,   'i', 0
    ,   'c', 0
    ,   'k', 0
    ,   ' ', 0
    ,   'A', 0
    ,   'p', 0
    ,   'p', 0
    ,   'l', 0
    ,   'e', 0
    ,   't', 0
    ,   'o', 0
    ,   'n', 0
    };

static
const unsigned char midi_prod_str[] =
    {   40
    ,   USB_DESC_TYPE_STRING
    ,   'U', 0
    ,   'S', 0
    ,   'B', 0
    ,   ' ', 0
    ,   'M', 0
    ,   'I', 0
    ,   'D', 0
    ,   'I', 0
    ,   ' ', 0
    ,   'C', 0
    ,   'o', 0
    ,   'n', 0
    ,   't', 0
    ,   'r', 0
    ,   'o', 0
    ,   'l', 0
    ,   'l', 0
    ,   'e', 0
    ,   'r', 0
    };

static
const unsigned char* midi_strings[] =
{   0 /* should return string table 9.6.7 */
,   midi_mfg_str
,   midi_prod_str
};

#define NB_STRINGS (sizeof(midi_strings) / sizeof(unsigned char*))

static
const unsigned char*
midi_get_string_desc
    (unsigned string_id
    ,unsigned lang_id
    )
{
    return (((lang_id == 0) || (lang_id == 0x409)) && (string_id < NB_STRINGS)) ? midi_strings[string_id] : 0;
}

static
const unsigned char*
midi_get_cfg_desc
    (unsigned index
    )
{
    return (index == 0) ? midi_conf_desc : 0;
}

volatile unsigned the_mode = 0;

static
void
midi_frame_event
    (unsigned frame_index
    )
{
    if ((the_mode == 1) || (the_mode == 2))
    {
        usb_sie_set_mode(1 << 5 /* INAK_BI */);
    }
}

static
void
midi_endpoint_event
    (unsigned physical_endpoint
    )
{
    if (physical_endpoint & 1)
    {
        /*FIXME for testing - should be removed */
        if (the_mode == 1)
        {
            static const unsigned char buf[8] = {0x09, 0x90, 60, 0x7f, 0x09, 0x90, 64, 0x7f};
            usb_write(physical_endpoint, buf, 8);
            the_mode = 0;
        }
        else if (the_mode == 2)
        {
            static const unsigned char buf[4] = {0x08, 0x80, 60, 0x00};
            usb_write(physical_endpoint, buf, 4);
            the_mode = 0;
        }
        else
        {
            usb_sie_set_mode(0); /* disable interrupt on NAKs... */
        }
    }
    else
    {
        static unsigned char pdata[64];
        (void)usb_read
            (physical_endpoint
            ,pdata
            ,sizeof(pdata)
            );
    }
}

static
const struct usb_configuration_s midi_config =
{   midi_desc
,   midi_get_cfg_desc
,   midi_get_string_desc
,   midi_frame_event
,   midi_endpoint_event
};

void
usb_midi_setup
    (unsigned long fosc
    )
{
    usb_setup(fosc, &midi_config);

    {
        /* for testing. a slow loop which will trigger note on/off messages
         * that never ends. */
        unsigned i = 0;
        while (1)
        {
            if (usb_is_configured())
            {
                i++;
                if (i == 10000000)
                {
                    the_mode = 1;
                }
                else if (i == 20000000)
                {
                    the_mode = 2;
                    i = 0;
                }
            }
        }
    }

}


