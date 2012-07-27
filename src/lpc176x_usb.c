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
#include "lpc176x_pll1.h"
#include "lpc176x_usb_pvt.h"
#include "lpc176x_usb_sie.h"
#include "LPC17xx.h"
#include "debughlprs.h"

/* FIXME: this module does not perform resets properly... g_device_state always
 * remains configured after a configuration. */

/* USB 2.0 Spec table 9-5 */

#define PCONP_PCUSB             (1ul << 31)

#define USBCLKCTRL_DEV_CLK_EN   (1 << 1)
#define USBCLKCTRL_AHB_CLK_EN   (1 << 4)
#define USBCLKST_DEV_CLK_ON     (1 << 1)
#define USBCLKST_AHB_CLK_EN     (1 << 4)

#define USB_DEV_STATUS_CON      (1 << 0)
#define USB_DEV_STATUS_CON_CH   (1 << 1)
#define USB_DEV_STATUS_SUS      (1 << 2)
#define USB_DEV_STATUS_SUS_CH   (1 << 3)
#define USB_DEV_STATUS_RST      (1 << 4)

#define USB_RX_PKT_RDY          (1 << 11)
#define USB_RX_PKT_VALID        (1 << 10)

#define USB_CTRL_RD_EN          (1 << 0)
#define USB_CTRL_WR_EN          (1 << 1)
#define USB_CTRL_LOG_ENDP(x)    (((x) & 0xf) << 2)

#define USB_PKTST_FE            (1 << 0)
#define USB_PKTST_ST            (1 << 1)
#define USB_PKTST_STP           (1 << 2)
#define USB_PKTST_PO            (1 << 3)
#define USB_PKTST_EPN           (1 << 4)

/* From USB2.0 chapter 9.1 (these are the important states) */
enum { USB_STATE_DEFAULT, USB_STATE_ADDRESS, USB_STATE_CONFIGURED }     g_device_state;
const struct usb_configuration_s                                       *g_config_descriptor;

/* Information about the endpoints. This is populated automatically from the
 * g_config_descriptor data structure on load. */
struct endpoint_descriptor_s
{
    int             enabled;
    unsigned        max_buffer_size;
} g_endpoint_descriptors[32];

int
usb_is_configured
    (void
    )
{
    return (g_device_state == USB_STATE_CONFIGURED);
}

unsigned
usb_write
    (unsigned               physical_endpoint
    ,const unsigned char   *buffer
    ,unsigned               size
    )
{
    unsigned to_write = g_endpoint_descriptors[physical_endpoint].max_buffer_size;
    unsigned i;
    unsigned long val = 0;
    ASSERT(physical_endpoint & 1);
    if (to_write > size)
    {
        to_write = size;
    }
    LPC_USB->USBCtrl = USB_CTRL_WR_EN | ((physical_endpoint << 1) & 0x3c);
    LPC_USB->USBTxPLen = to_write;
    for (i = 0; i < to_write; i++)
    {
        val = (val >> 8);
        if (buffer)
        {
            val = val | (((unsigned long)buffer[i]) << 24);
        }
        if ((i & 3) == 3)
        {
            LPC_USB->USBTxData = val;
        }
    }
    i = i & 3;
    if (i)
    {
        LPC_USB->USBTxData = val >> (8 * (4 - i));
    }
    LPC_USB->USBCtrl = 0;
    usb_sie_select_endpoint(physical_endpoint);
    usb_sie_validate_buffer();
    return to_write;
}

int
usb_read
    (unsigned       physical_endpoint
    ,unsigned char *buffer
    ,unsigned       buffer_size
    )
{
    unsigned long rx_plen;
    int status = -1;
    ASSERT((physical_endpoint & 1) == 0);
    LPC_USB->USBCtrl = USB_CTRL_RD_EN | ((physical_endpoint << 1) & 0x3c);
    do
    {
        rx_plen = LPC_USB->USBRxPLen;
    } while ((rx_plen & USB_RX_PKT_RDY) == 0);
    if (rx_plen & USB_RX_PKT_VALID)
    {
        const unsigned packet_len = (rx_plen & 0x3ff);
        unsigned i;
        unsigned long data = LPC_USB->USBRxData;
        for (i = 0; i < packet_len; i++)
        {
            if ((i < buffer_size) && (buffer))
            {
                buffer[i] = data & 0xff;
            }
            if ((i & 0x3) == 3)
            {
                data = LPC_USB->USBRxData;
            }
            else
            {
                data >>= 8;
            }
        }
        status = (buffer_size < packet_len) ? buffer_size : packet_len;
    }
    LPC_USB->USBCtrl = 0;
    usb_sie_select_endpoint(physical_endpoint);
    usb_sie_clear_buffer();
    return status;
}

struct ctl_data_stream_s
{
    const unsigned char    *data;
    unsigned                data_left;
};

static
void
usb_send_control_stream
    (struct ctl_data_stream_s *stream
    )
{
    const unsigned written  = usb_write(1, stream->data, stream->data_left);
    stream->data           += written;
    stream->data_left      -= written;
}

#define GET_REQ_IS_H2D(request_type) ((request_type) >> 7)
#define GET_REQ_TYPE(request_type) (((request_type) >> 5) & 0x3)
#define GET_REQ_RECIP(request_type) ((request_type) & 0x1f)

#define REQ_TYPE_STANDARD (0)
#define REQ_RECIP_DEVICE (0)

#define REQ_CODE_SET_ADDRESS       (5)
#define REQ_CODE_GET_DESCRIPTOR    (6)
#define REQ_CODE_SET_CONFIGURATION (9)

/* USB2.0 9.4.3 - Get Descriptor */
static
int
usb_control_get_descriptor
    (struct ctl_data_stream_s  *stream
    ,unsigned                   wvalue
    ,unsigned                   windex
    ,unsigned                   wlength
    )
{
    /* standard reqeust - device - host to device */
    const unsigned desc_index  = wvalue & 0xff;
    const unsigned desc_type   = wvalue >> 8;
    const unsigned char* data = 0;
    switch (desc_type)
    {
    case USB_DESC_TYPE_DEVICE:
        data = g_config_descriptor->dev_desc;
        break;
    case USB_DESC_TYPE_CONFIGURATION:
        data = g_config_descriptor->get_cfg_desc(desc_index);
        break;
    case USB_DESC_TYPE_STRING:
        data = g_config_descriptor->get_string_desc(desc_index, windex);
        break;
    case USB_DESC_TYPE_INTERFACE:
    case USB_DESC_TYPE_ENDPOINT:
    case USB_DESC_TYPE_DEVICE_QUALIFIER:
    case USB_DESC_TYPE_OTHER_SPEED_CFG:
    case USB_DESC_TYPE_INTERFACE_POWER:
    default:
        break;
    }
    if (data)
    {
        const unsigned desc_size =
            (desc_type == USB_DESC_TYPE_CONFIGURATION)
            ? ((((unsigned)data[3]) << 8) | data[2])
            : data[0];
        stream->data        = data;
        stream->data_left   = (wlength > desc_size) ? desc_size : wlength;
        return 1;
    }
    return 0;
}

/* USB2.0 9.4.6 - Set Address */
static
int
usb_control_set_address
    (unsigned                   wvalue
    ,unsigned                   windex
    ,unsigned                   wlength
    )
{
    if (((g_device_state == USB_STATE_DEFAULT) || (g_device_state == USB_STATE_ADDRESS)) && (wvalue < 128) && (windex == 0) && (wlength == 0))
    {
        usb_sie_set_address(wvalue);
        g_device_state = (wvalue) ? USB_STATE_ADDRESS : USB_STATE_DEFAULT;
        return 1;
    }
    return 0;
}

static
void
realize_endpoint
    (unsigned endpoint
    ,unsigned max_packet_size
    )
{
    LPC_USB->USBDevIntClr  = USB_DI_EP_RLZD;
    LPC_USB->USBReEp      |= (1ul << endpoint);
    LPC_USB->USBEpInd      = endpoint;
    LPC_USB->USBMaxPSize   = max_packet_size;
    while (!(LPC_USB->USBDevIntSt & USB_DI_EP_RLZD));
}

static
void
realize_and_enable_endpoint
    (unsigned endpoint
    ,unsigned max_packet_size
    )
{
    realize_endpoint(endpoint, max_packet_size);
    usb_sie_enable_endpoint(endpoint);
    g_endpoint_descriptors[endpoint].enabled = 1;
    g_endpoint_descriptors[endpoint].max_buffer_size = max_packet_size;
}

static
int
usb_control_set_config_helper
    (const unsigned char* config
    )
{
    const unsigned cfg_len = (((unsigned)config[3]) << 8) | config[2];
    unsigned desc_idx;
    unsigned long ep_int_flags = 0x3;
    /* Enumerate over the USB configuration looking for endpoints */
    for (desc_idx = config[0]; desc_idx < cfg_len; desc_idx += config[desc_idx])
    {
        const unsigned char *descriptor = &(config[desc_idx]);
        if (descriptor[1] == USB_DESC_TYPE_ENDPOINT)
        {
            unsigned physical_endpoint = ((descriptor[2] & 0x0f) << 1) | (descriptor[2] >> 7);
            unsigned max_packet_size   = descriptor[4];
            ASSERT((physical_endpoint != 0) && (physical_endpoint != 1));
            realize_and_enable_endpoint(physical_endpoint, max_packet_size);
            ep_int_flags |= (1 << physical_endpoint);
        }
    }
    LPC_USB->USBEpIntEn = ep_int_flags;
    usb_sie_configure_device(1);
    g_device_state = USB_STATE_CONFIGURED;
    return 1;
}

/* USB2.0 9.4.7 - Set Configuration */
static
int
usb_control_set_configuration
    (unsigned                   wvalue
    ,unsigned                   windex
    ,unsigned                   wlength
    )
{
    if ((windex == 0) && (wlength == 0) && ((g_device_state == USB_STATE_ADDRESS) || (g_device_state == USB_STATE_CONFIGURED)))
    {
        if (wvalue == 0)
        {
            usb_sie_configure_device(0);
            g_device_state = USB_STATE_ADDRESS;
            return 1;
        }
        else
        {
            unsigned idx = 0;
            const unsigned char* cfg = g_config_descriptor->get_cfg_desc(idx);
            while (cfg)
            {
                if (cfg[5] == wvalue)
                {
                    return usb_control_set_config_helper(cfg);
                }
                idx++;
            }
        }
    }
    return 0;
}

/* USB2.0 9.4.8 - Set Descriptor
 *     This method is optional.
 */

static
int
usb_handle_control_read
    (struct ctl_data_stream_s  *stream
    ,const unsigned char       *packet_buf
    ,unsigned                   packet_size
    )
{
    int handled = 0;
    if (packet_size == 8)
    {
        const unsigned  wvalue  = (((unsigned)packet_buf[3]) << 8) | packet_buf[2];
        const unsigned  windex  = (((unsigned)packet_buf[5]) << 8) | packet_buf[4];
        const unsigned  wlength = (((unsigned)packet_buf[7]) << 8) | packet_buf[6];
        if (GET_REQ_TYPE(packet_buf[0]) == REQ_TYPE_STANDARD)
        {
            if (GET_REQ_RECIP(packet_buf[0]) == REQ_RECIP_DEVICE)
            {
                if ((GET_REQ_IS_H2D(packet_buf[0])) && (packet_buf[1] == REQ_CODE_GET_DESCRIPTOR))
                {
                    handled = usb_control_get_descriptor(stream, wvalue, windex, wlength);
                }
                else if ((!GET_REQ_IS_H2D(packet_buf[0])) && (packet_buf[1] == REQ_CODE_SET_CONFIGURATION))
                {
                    handled = usb_control_set_configuration(wvalue, windex, wlength);
                }
                else if ((!GET_REQ_IS_H2D(packet_buf[0])) && (packet_buf[1] == REQ_CODE_SET_ADDRESS))
                {
                    handled = usb_control_set_address(wvalue, windex, wlength);
                }
            }
        }
    }
    return handled;
}

static
void
usb_handle_ep_int
    (unsigned       physical_endpoint
    ,unsigned       flags
    )
{
    if (physical_endpoint == 0 || physical_endpoint == 1)
    {
        static unsigned char pdata[64];
        static struct ctl_data_stream_s ctl_stream;
        if (physical_endpoint & 1)
        {
            usb_send_control_stream(&ctl_stream);
        }
        else
        {
            if (flags & USB_PKTST_STP)
            {
                int plen =
                    usb_read
                        (0
                        ,pdata
                        ,8
                        );
                if (plen >= 0)
                {
                    unsigned wlen = pdata[6] | (((unsigned)pdata[7]) << 8);
                    if ((wlen == 0) || (pdata[0] & 0x80)) /* USB2.0 - section 9.3.1 */
                    {
                        int handled =
                            usb_handle_control_read
                                (&ctl_stream
                                ,pdata
                                ,plen
                                );
                        if (!handled)
                        {
                            dump_buffer(pdata, plen);
                        }
                    }
                    usb_send_control_stream
                        (&ctl_stream
                        );
                }
                else
                {
                    usb_sie_stall_endpoint(0);
                }
            }
            else
            {
                (void)usb_read(0, 0, 0);
            }
        }
    }
    else
    {
        if (g_config_descriptor->on_usb_endpoint)
        {
            g_config_descriptor->on_usb_endpoint(physical_endpoint);
        }
        else
        {
            usb_sie_stall_endpoint(0);
        }
    }
}

static
void
usb_reset
    (void
    )
{
    g_device_state                              = USB_STATE_DEFAULT;
    LPC_USB->USBDevIntClr                       = 0xfffffffful;
    LPC_USB->USBEpIntClr                        = 0xfffffffful;
    realize_and_enable_endpoint(0, g_config_descriptor->dev_desc[7]);
    realize_and_enable_endpoint(1, g_config_descriptor->dev_desc[7]);
    LPC_USB->USBDevIntEn                        = USB_DI_FRAME | USB_DI_EP_SLOW | USB_DI_DEV_STAT;
    LPC_USB->USBEpIntEn                         = 0x3;
    usb_sie_set_address(0);
}

void
USB_IRQHandler
    (void
    )
{
    const unsigned long interrupt_flags = LPC_USB->USBDevIntSt;
    const unsigned long endpoint_flags = (interrupt_flags & USB_DI_EP_FAST) | (interrupt_flags & USB_DI_EP_SLOW);
    LPC_USB->USBDevIntClr = interrupt_flags;
    if ((interrupt_flags & USB_DI_FRAME) && (g_config_descriptor->on_usb_frame))
    {
        g_config_descriptor->on_usb_frame(usb_sie_get_frame_number());
    }
    if (interrupt_flags & USB_DI_DEV_STAT)
    {
        unsigned stat_flags = usb_sie_get_device_status();
        if (stat_flags & USB_DEV_STATUS_RST)
        {
            usb_reset();
        }
    }
    if (endpoint_flags)
    {
        unsigned i;
        unsigned long ep_mask_bit = 1;
        for (i = 0; (LPC_USB->USBEpIntSt) && (i < 32); i++, ep_mask_bit <<= 1)
        {
            if (LPC_USB->USBEpIntSt & ep_mask_bit)
            {
                unsigned cmd_data;
                LPC_USB->USBEpIntClr    = ep_mask_bit;
                while ((LPC_USB->USBDevIntSt & USB_DI_CDFULL) != USB_DI_CDFULL);
                LPC_USB->USBDevIntClr   = USB_DI_CDFULL;
                cmd_data                = LPC_USB->USBCmdData & 0xff;
                usb_handle_ep_int(i, cmd_data);
            }
        }
    }
}

void
usb_setup
    (unsigned long                      fosc
    ,const struct usb_configuration_s  *usb_config
    )
{
    /* Store the global USB hardware descriptor */
    g_config_descriptor = usb_config;

    /* Select USB pins and their modes in PINSEL0 to PINSEL5 and PINMODE0 to
     * PINMODE5.
     *
     * P0[29]   USB_D+        I/O   Positive differential data USB Connector
     * P0[30]   USB_D−        I/O   Negative differential data USB Connector
     * P2[9]    USB_CONNECT   O     SoftConnect control signal.
     * P1[18]   USB_UP_LED    O     GoodLink LED control signal Control
     * P1[30]   VBUS          I     VBUS status input. When this function is
     *                              not enabled via its corresponding PINSEL
     *                              register, it is driven HIGH internally.
     *                              pullups/pulldowns must be manually disabled
     *                              on this pin. */
    LPC_PINCON->PINSEL1  = (LPC_PINCON->PINSEL1  & ~0x3c000000ul) | 0x14000000ul; /* USB D+ and USB D- */
    LPC_PINCON->PINSEL3  = (LPC_PINCON->PINSEL3  & ~0x30000030ul) | 0x20000010ul; /* Vbus and USB_UP_LED */
    LPC_PINCON->PINSEL4  = (LPC_PINCON->PINSEL4  & ~0x000c0000ul) | 0x00040000ul; /* USB_CONNECT */
    LPC_PINCON->PINMODE3 = (LPC_PINCON->PINMODE3 & ~0xc0000000ul) | 0x40000000ul; /* Vbus pullups off */

    /* Enable power to USB module. */
    LPC_SC->PCONP |= PCONP_PCUSB;

    /* 2) Setup PLL1 to be the clock source for USB and wait for the clock to
     * be available. */
    pll1_setup(fosc);
    LPC_USB->USBClkCtrl = (USBCLKCTRL_DEV_CLK_EN | USBCLKCTRL_AHB_CLK_EN);
    while (!(LPC_USB->USBClkSt & (USBCLKST_DEV_CLK_ON | USBCLKST_AHB_CLK_EN)));

    /* 6) Realize endpoints */
    /* 7. Enable endpoint interrupts (Slave mode):
    * – Clear all endpoint interrupts using USBEpIntClr.
    * – Clear any device interrupts using USBDevIntClr.
    * – Enable Slave mode for the desired endpoints by setting the
    *   corresponding bits in USBEpIntEn.
    * – Set the priority of each enabled interrupt using USBEpIntPri.
    * – Configure the desired interrupt mode using the SIE Set Mode command.
    * – Enable device interrupts using USBDevIntEn (normally DEV_STAT,
    *   EP_SLOW, and possibly EP_FAST)
    */
    usb_reset();
    usb_sie_set_mode(0);
    NVIC_SetPriority(USB_IRQn, 5);
    NVIC_EnableIRQ(USB_IRQn);

    /* 11. Set CON bit to 1 to make CONNECT active using the SIE Set Device
     * Status command.
     */
    usb_sie_set_device_status(USB_DEV_STATUS_CON);
}

