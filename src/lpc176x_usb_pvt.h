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

#ifndef USB_PVT_H_
#define USB_PVT_H_

/* Device Interrupt Bits */
#define USB_DI_FRAME            (0x001)
#define USB_DI_EP_FAST          (0x002)
#define USB_DI_EP_SLOW          (0x004)
#define USB_DI_DEV_STAT         (0x008)
#define USB_DI_CCEMPTY          (0x010)
#define USB_DI_CDFULL           (0x020)
#define USB_DI_RX_ENDPKT        (0x040)
#define USB_DI_TX_ENDPKT        (0x080)
#define USB_DI_EP_RLZD          (0x100)
#define USB_DI_ERR_INT          (0x200)

#endif /* LPC176X_USB_PVT_H_ */
