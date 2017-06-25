#ifndef USB_MACROS_H
#define USB_MACROS_H

#include <stm32f7xx.h>

#define DEVICE(usb)	((USB_OTG_DeviceTypeDef *)((void *)(usb) + USB_OTG_DEVICE_BASE))
#define INEP(usb, n)	((USB_OTG_INEndpointTypeDef *)((void *)(usb) + USB_OTG_IN_ENDPOINT_BASE + ((n) << 5)))
#define OUTEP(usb, n)	((USB_OTG_OUTEndpointTypeDef *)((void *)(usb) + USB_OTG_OUT_ENDPOINT_BASE + ((n) << 5)))
#define PCGCCTL(usb)	(*((__IO uint32_t *)((void *)(usb) + USB_OTG_PCGCCTL_BASE)))

#define DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ     (0 << 1)
#define DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ     (1 << 1)
#define DSTS_ENUMSPD_LS_PHY_6MHZ               (2 << 1)
#define DSTS_ENUMSPD_FS_PHY_48MHZ              (3 << 1)

#endif // USB_MACROS_H
