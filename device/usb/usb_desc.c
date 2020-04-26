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

static const desc_dev_t desc_dev = {
	18u, DESC_DEVICE, 0x0200u, 0xefu, 0x02u, 0x01u, 64u,
	USB_VID, USB_PID, USB_RELEASE, 0u, 0u, 0u, 1u
};

static void usb_get_descriptor_device(usb_desc_t *desc)
{
#ifdef DEBUG
	if (!desc->size || !desc->p)
		panic();
#endif
	desc_dev_t *pd = (desc_dev_t *)desc->p;
	memcpy(pd, &desc_dev, desc_dev.bLength);
	pd->bMaxPacketSize0 = usb_hw_ep_max_size(EP_DIR_IN, 0);
	pd->iManufacturer = usb_desc_add_string(0, LANG_EN_US, USB_MANUFACTURER);
	pd->iProduct = usb_desc_add_string(0, LANG_EN_US, USB_PRODUCT_NAME);

	// Construct serial number from UID
	char uid[25];
	uid_str(uid);
	// Valid serial number characters are [0-9A-F]
	pd->iSerialNumber = usb_desc_add_string(0, LANG_EN_US, uid);
#ifdef DEBUG
	if (pd->bLength > desc->size)
		panic();
#endif
	desc->size = pd->bLength;
}

typedef struct desc_string_list_t {
	struct desc_string_list_t *next;
	uint32_t lang;
	usb_desc_t desc;
} desc_string_list_t;

struct {
	usb_desc_t lang;
	uint32_t nstring;
	desc_string_list_t **string;
} desc = {0};

typedef struct PACKED {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wPayload[];
} desc_string_t;

static const desc_string_t desc_string = {2u, DESC_STRING};

uint32_t usb_desc_add_string(uint16_t id, uint16_t lang, const char *str)
{
	// Update language list
	if (!desc.lang.size) {
		// New language list
		desc_string_t *pl = (desc_string_t *)malloc(desc_string.bLength + 2);
		if (!pl)
			panic();
		memcpy(pl, &desc_string, desc_string.bLength);
		pl->wPayload[0] = lang;
		pl->bLength += 2;
		desc.lang.p = pl;
		desc.lang.size = pl->bLength;
	} else {
		// Append only if not exists
		desc_string_t *pl = (desc_string_t *)desc.lang.p;
		for (uint8_t len = 0; len != pl->bLength - desc_string.bLength; len += 2)
			if (pl->wPayload[len] == lang)
				goto add;
		pl = (desc_string_t *)realloc(pl, pl->bLength + 2);
		pl->wPayload[(pl->bLength - desc_string.bLength) >> 1] = lang;
		pl->bLength += 2;
		desc.lang.p = pl;
		desc.lang.size = pl->bLength;
	}

add:	// Add string to string lists
	if (id > desc.nstring)
		id = 0;

	// Find string list entry
	desc_string_list_t **psl;
	if (id == 0) {	// Append new string entry
		desc.string = (desc_string_list_t **)realloc(desc.string, sizeof(desc_string_list_t *) * (desc.nstring + 1u));
		psl = &desc.string[desc.nstring++];
		id = desc.nstring;
		*psl = 0;
	} else
		psl = &desc.string[id - 1u];

	// Iterate to end of string list
	// TODO: Duplication check
	for (; *psl != 0; psl = &(*psl)->next);
	// Add new string entry
	*psl = (desc_string_list_t *)malloc(sizeof(desc_string_list_t));
	if (!psl)
		panic();
	(*psl)->next = 0;
	(*psl)->lang = lang;
	// Allocate string descriptor
	uint32_t len = strlen(str);
	desc_string_t *ps = (desc_string_t *)malloc(desc_string.bLength + (len << 1u));
	if (!ps)
		panic();
	memcpy(ps, &desc_string, desc_string.bLength);
	ps->bLength += (len << 1u);
	while (len--)	// Unicode conversion?
		ps->wPayload[len] = str[len];
	(*psl)->desc.p = ps;
	(*psl)->desc.size = ps->bLength;
	return id;
}

void usb_get_descriptor(usb_setup_t pkt, usb_desc_t *desc)
{
	switch (pkt.bType) {
	case DESC_DEVICE:
		usb_get_descriptor_device(desc);
		break;
	case DESC_CONFIGURATION:
		USB_TODO();
		//desc = usb_desc_config(usb);
		break;
	case DESC_DEVICE_QUALIFIER:
		USB_TODO();
		//desc = usb_desc_device_qualifier(usb);
		break;
	case DESC_STRING:
		USB_TODO();
		//desc = usb_desc_string(usb, pkt.bIndex, pkt.wIndex);
		break;
	default:
		USB_TODO();
		break;
	}
}
