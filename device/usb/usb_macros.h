#ifndef USB_MACROS_H
#define USB_MACROS_H

#include <stm32f7xx.h>
#include "../macros.h"

#define FUNC(f)	if (f) f

#define DEVICE(usb)	((USB_OTG_DeviceTypeDef *)((void *)(usb) + USB_OTG_DEVICE_BASE))
#define EP_IN(usb, n)	((USB_OTG_INEndpointTypeDef *)((void *)(usb) + USB_OTG_IN_ENDPOINT_BASE + ((n) << 5)))
#define EP_OUT(usb, n)	((USB_OTG_OUTEndpointTypeDef *)((void *)(usb) + USB_OTG_OUT_ENDPOINT_BASE + ((n) << 5)))
#define PCGCCTL(usb)	(*((__IO uint32_t *)((void *)(usb) + USB_OTG_PCGCCTL_BASE)))
#define FIFO(usb, n)	(*((__IO uint32_t *)((void *)(usb) + USB_OTG_FIFO_BASE + USB_OTG_FIFO_SIZE * (n))))

#define DAINTMSK_IN(n)	((1ul << (n)) << USB_OTG_DAINTMSK_IEPM_Pos)
#define DAINTMSK_OUT(n)	((1ul << (n)) << USB_OTG_DAINTMSK_OEPM_Pos)

#define DIEPCTL_MASK	(USB_OTG_DIEPCTL_TXFNUM_Msk | USB_OTG_DIEPCTL_EPTYP_Msk | \
	USB_OTG_DIEPCTL_USBAEP_Msk | USB_OTG_DIEPCTL_MPSIZ_Msk)
#define DIEPCTL_SET(r, m)	(r) = ((r) & DIEPCTL_MASK) | (m)
#define DOEPCTL_MASK	(USB_OTG_DOEPCTL_SNPM_Msk | USB_OTG_DOEPCTL_EPTYP_Msk | \
	USB_OTG_DOEPCTL_USBAEP_Msk | USB_OTG_DOEPCTL_MPSIZ_Msk)
#define DOEPCTL_SET(r, m)	(r) = ((r) & DOEPCTL_MASK) | (m)
#define DIEPTXF(a, d)	((((d) >> 2) << USB_OTG_DIEPTXF_INEPTXFD_Pos) | ((a) << USB_OTG_DIEPTXF_INEPTXSA_Pos))

#define EP_IN_TYP_CONTROL	(0b00ul << USB_OTG_DIEPCTL_EPTYP_Pos)
#define EP_IN_TYP_ISOCHRONOUS	(0b01ul << USB_OTG_DIEPCTL_EPTYP_Pos)
#define EP_IN_TYP_BULK		(0b10ul << USB_OTG_DIEPCTL_EPTYP_Pos)
#define EP_IN_TYP_INTERRUPT	(0b11ul << USB_OTG_DIEPCTL_EPTYP_Pos)

#define STAT_OUT_NAK	(0b0001ul << USB_OTG_GRXSTSP_PKTSTS_Pos)
#define STAT_OUT_RECV	(0b0010ul << USB_OTG_GRXSTSP_PKTSTS_Pos)
#define STAT_OUT	(0b0011ul << USB_OTG_GRXSTSP_PKTSTS_Pos)
#define STAT_SETUP	(0b0100ul << USB_OTG_GRXSTSP_PKTSTS_Pos)
#define STAT_SETUP_RECV	(0b0110ul << USB_OTG_GRXSTSP_PKTSTS_Pos)

#define STAT_EP(s)	(((s) & USB_OTG_GRXSTSP_EPNUM_Msk) >> USB_OTG_GRXSTSP_EPNUM_Pos)
#define STAT_CNT(s)	(((s) & USB_OTG_GRXSTSP_BCNT_Msk) >> USB_OTG_GRXSTSP_BCNT_Pos)

#endif // USB_MACROS_H
