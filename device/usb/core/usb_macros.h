#ifndef USB_MACROS_H
#define USB_MACROS_H

#include <device.h>
#include <defines.h>

#define FUNC(f)	if (f) f

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
#define DIEPTXF(a, d)	((((d) >> 2) << USB_OTG_DIEPTXF_INEPTXFD_Pos) | ((a) << USB_OTG_DIEPTXF_INEPTXSA_Pos))

#define EP_TYP_CONTROL		(0b00ul << USB_OTG_DIEPCTL_EPTYP_Pos)
#define EP_TYP_ISOCHRONOUS	(0b01ul << USB_OTG_DIEPCTL_EPTYP_Pos)
#define EP_TYP_BULK		(0b10ul << USB_OTG_DIEPCTL_EPTYP_Pos)
#define EP_TYP_INTERRUPT	(0b11ul << USB_OTG_DIEPCTL_EPTYP_Pos)

// Endpoint direction
#define EP_DIR_Msk	0x80
#define EP_DIR_IN	0x80
#define EP_DIR_OUT	0x00

// Transfer type
#define EP_CONTROL	0x00
#define EP_ISOCHRONOUS	0x01
#define EP_BULK		0x02
#define EP_INTERRUPT	0x03

// Iso mode synchronisation type
#define EP_ISO_NONE	0x00
#define EP_ISO_ASYNC	0x04
#define EP_ISO_ADAPTIVE	0x08
#define EP_ISO_SYNC	0x0c

// Iso mode usage type
#define EP_ISO_DATA	0x00
#define EP_ISO_FEEDBACK	0x10
#define EP_ISO_IMPLICIT	0x20

// Language IDs
#define LANG_ZH_CN	0x0804
#define LANG_EN_US	0x0409
#define LANG_EN_GB	0x0809

#endif // USB_MACROS_H
