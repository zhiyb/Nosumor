#include <malloc.h>
#include <string.h>
#include <usb/usb_macros.h>
#include <usb/usb_hw.h>
#include <usb/usb_desc.h>
#include <common.h>

#define DESC_DEVICE			1u
#define DESC_CONFIGURATION		2u
#define DESC_STRING			3u
#define DESC_INTERFACE			4u
#define DESC_ENDPOINT			5u
#define DESC_DEVICE_QUALIFIER		6u
#define DESC_OTHER_SPEED_CONFIGURATION	7u
#define DESC_INTERFACE_POWER		8u
#define DESC_OTG			9u
//#define DESC_DEBUG			10u
#define DESC_INTERFACE_ASSOCIATION	11u

// Device descriptor

typedef struct PACKED {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
} desc_dev_t;

LIST(usb_desc_string, usb_desc_string_t);

static char uid[25] = {0};

USB_DESC_STRING(desc_str_manufacturer) = {0, LANG_EN_US, USB_MANUFACTURER};
USB_DESC_STRING(desc_str_product) = {0, LANG_EN_US, USB_PRODUCT_NAME};
USB_DESC_STRING(desc_str_serial) = {0, LANG_EN_US, uid};

static const desc_dev_t desc_dev = {
	18u, DESC_DEVICE, 0x0200u, 0xefu, 0x02u, 0x01u, 64u,
	USB_VID, USB_PID, USB_RELEASE, 0u, 0u, 0u, 1u
};

static void usb_desc_device(usb_desc_t *pdesc)
{
#if DEBUG >= 5
	if (!pdesc->size || !pdesc->p)
		USB_ERROR();
#endif
	desc_dev_t *pd = (desc_dev_t *)pdesc->p;
	memcpy(pd, &desc_dev, desc_dev.bLength);
	pd->bMaxPacketSize0 = usb_hw_ep_max_size(EP_DIR_IN, 0);
	pd->iManufacturer = USB_DESC_STRING_INDEX(desc_str_manufacturer);
	pd->iProduct = USB_DESC_STRING_INDEX(desc_str_product);

	// Construct serial number from UID
	// Valid serial number characters are [0-9A-F]
	if (uid[0] == 0)
		uid_str(uid);
	pd->iSerialNumber = USB_DESC_STRING_INDEX(desc_str_serial);
#if DEBUG >= 5
	if (pd->bLength > pdesc->size)
		USB_ERROR();
#endif
	pdesc->size = pd->bLength;
}

// Configration descriptor

typedef struct PACKED {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} desc_config_t;

enum {ConfigAttributeSelfPowered = 1u << 6u,
	ConfigAttributeRemoteWakeup = 1u << 5u};

static const desc_config_t desc_config = {
	9u, DESC_CONFIGURATION, 9u,
	0u, 1u, 0u, 0x80 | ConfigAttributeRemoteWakeup, 250,
};

LIST(usb_desc_config, usb_desc_handler_t);

static void usb_desc_config(usb_desc_t *pdesc)
{
#if DEBUG >= 5
	if (!pdesc->size || !pdesc->p)
		USB_ERROR();
#endif
	// Use buffer on stack for unaligned access
	union {
		desc_config_t desc;
		uint8_t raw[pdesc->size];
	} conf;
	conf.desc = desc_config;
	usb_desc_t desc;
	desc.p = &conf;
	desc.size = pdesc->size;
	LIST_ITERATE(usb_desc_config, usb_desc_handler_t, pfunc) (*pfunc)(&desc);
#if DEBUG >= 5
	if (conf.desc.wTotalLength > pdesc->size)
		USB_ERROR();
#endif
	memcpy(pdesc->p, &conf, conf.desc.wTotalLength);
	pdesc->size = conf.desc.wTotalLength;
}

// Interface descriptor

typedef struct PACKED {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} desc_interface_t;

uint8_t usb_desc_add_interface(usb_desc_t *pdesc, uint8_t bAlternateSetting, uint8_t bNumEndpoints,
			    uint8_t bInterfaceClass, uint8_t bInterfaceSubClass,
			    uint8_t bInterfaceProtocol, uint8_t iInterface)
{
	static const uint8_t bLength = 9u;
	desc_config_t *pc = (desc_config_t *)pdesc->p;
	uint8_t bNumInterfaces = pc->bNumInterfaces;
	const desc_interface_t desc = {
		bLength, DESC_INTERFACE, bNumInterfaces, bAlternateSetting, bNumEndpoints,
		bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol, iInterface
	};
	usb_desc_add(pdesc, &desc, bLength);
	pc->bNumInterfaces = bNumInterfaces + 1;
	return bNumInterfaces;
}

// Endpoint descriptor

typedef struct PACKED {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
} desc_ep_t;

void usb_desc_add_endpoint(usb_desc_t *pdesc, uint8_t bEndpointAddress, uint8_t bmAttributes,
			   uint16_t wMaxPacketSize, uint8_t bInterval)
{
	static const uint8_t bLength = 7u;
	const desc_ep_t desc = {
		bLength, DESC_ENDPOINT, bEndpointAddress, bmAttributes, wMaxPacketSize, bInterval
	};
	usb_desc_add(pdesc, &desc, bLength);
}

// Custom descriptor

void usb_desc_add(usb_desc_t *pdesc, const void *p, uint32_t bLength)
{
#if DEBUG >= 5
	if (pdesc->size < bLength)
		USB_ERROR();
#endif
	desc_config_t *pc = (desc_config_t *)pdesc->p;
	memcpy(pdesc->p + pc->wTotalLength, p, bLength);
	pc->wTotalLength += bLength;
	pdesc->size -= bLength;
}

// String descriptor

typedef struct PACKED {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wPayload[];
} desc_string_t;

static const desc_string_t desc_string = {2u, DESC_STRING};

static void usb_desc_string(usb_desc_t *pdesc, uint8_t index, uint16_t lang)
{
	// Language list or out-of-range
	if (index == 0) {
		union {
			desc_string_t desc;
			uint16_t raw[33];
		} lang;
		lang.desc = desc_string;
		lang.desc.wPayload[0] = 0;
		LIST_ITERATE(usb_desc_string, usb_desc_string_t, pstr) {
			usb_desc_string_t *ps = pstr;
			do {
				uint16_t *plang = &lang.raw[1];
				while (*plang != 0)
					if (*plang == ps->lang)
						break;
#if DEBUG >= 5
				if (plang == &lang.raw[32])
					USB_ERROR();
#endif
				if (*plang == 0) {
					*plang++ = ps->lang;
					*plang = 0;
				}
				ps = ps->next;
			} while (ps != 0);
		}
		uint16_t nlang = 0;
		for (uint16_t *plang = &lang.raw[1]; *plang != 0; plang++)
			nlang++;
		lang.desc.bLength += 2 * nlang;
		memcpy(pdesc->p, &lang, lang.desc.bLength);
		pdesc->size = lang.desc.bLength;
	} else if (index <= LIST_SIZE(usb_desc_string)) {
		usb_desc_string_t *ps = &LIST_AT(usb_desc_string, index);
		do {
			if (ps->lang == lang)
				break;
			ps = ps->next;
		} while (ps != 0);
		if (ps == 0)
			ps = &LIST_AT(usb_desc_string, index);

		uint16_t len = strlen(ps->p);
		union {
			desc_string_t desc;
			uint8_t raw[desc_string.bLength + len * 2];
		} desc;
		desc.desc = desc_string;
		desc.desc.bLength += len * 2;
		while (len--)	// Unicode conversion?
			desc.desc.wPayload[len] = ps->p[len];

#if DEBUG >= 5
		if (pdesc->size < desc.desc.bLength)
			USB_ERROR();
#endif
		memcpy(pdesc->p, &desc, desc.desc.bLength);
		pdesc->size = desc.desc.bLength;
	} else {
		// Default empty descriptor
		pdesc->size = 0;
		USB_TODO();
	}
}

void usb_get_descriptor(usb_desc_t *pdesc, usb_setup_t pkt)
{
	switch (pkt.bType) {
	case DESC_DEVICE:
		usb_desc_device(pdesc);
		break;
	case DESC_CONFIGURATION:
		usb_desc_config(pdesc);
		break;
	case DESC_DEVICE_QUALIFIER:
		USB_TODO();
		//desc = usb_desc_device_qualifier(usb);
		break;
	case DESC_STRING:
		usb_desc_string(pdesc, pkt.bIndex, pkt.wIndex);
		break;
	default:
		USB_TODO();
		break;
	}
}
