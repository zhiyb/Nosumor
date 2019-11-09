#ifndef USB_HW_MACROS_H
#define USB_HW_MACROS_H

#include <device.h>
#include <macros.h>
#include "usb_macros.h"

#define DEV(usb)	((USB_OTG_DeviceTypeDef *)((void *)(usb) + USB_OTG_DEVICE_BASE))
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
#define DIEPTXF(a, d)	(((d) << USB_OTG_DIEPTXF_INEPTXFD_Pos) | ((a) << USB_OTG_DIEPTXF_INEPTXSA_Pos))

// Value conversion from registers
#define DIEPCTL_EP_TYP(reg)	((reg & USB_OTG_DIEPCTL_EPTYP_Msk) >> USB_OTG_DIEPCTL_EPTYP_Pos)
#define DSTS_ENUM_SPD(reg)	((reg & USB_OTG_DSTS_ENUMSPD_Msk) >> USB_OTG_DSTS_ENUMSPD_Pos)

#endif // USB_HW_MACROS_H
