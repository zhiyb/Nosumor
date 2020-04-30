#include <string.h>
#include <usb/usb_macros.h>
#include <usb/usb_hw.h>
#include <usb/usb_desc.h>
#include <usb/ep0/usb_ep0.h>
#include <usb/ep0/usb_ep0_setup.h>
#include "usb_audio2_defs.h"
#include "usb_audio2_desc.h"
#include "usb_audio2.h"

#define EP_MAX_SIZE	(192u + 32u)

#define DESC_ENDPOINT			5u

static struct {
	struct {
		uint32_t iface;
	} ac;
	volatile uint32_t active;
} data;

USB_AUDIO2_ENTITY_LIST();
USB_AUDIO2_STREAMING_LIST();

// Allocate endpoints after enumeration

static void usb_audio2_enumeration(uint32_t spd)
{
	UNUSED(spd);
	data.active = 0;
	LIST_ITERATE(usb_audio2_streaming, usb_audio2_streaming_t, pas) {
		if (pas->type == UsbAudio2In) {
			pas->data->epnum = usb_hw_ep_alloc(UsbEpFast, EP_DIR_OUT, EP_ISOCHRONOUS, EP_MAX_SIZE);
			pas->data->fbepnum = usb_hw_ep_alloc(UsbEpFast, EP_DIR_IN, EP_ISOCHRONOUS, EP_MAX_SIZE);
		} else {
			USB_TODO();
		}

		// Allocate DMA memories
		pas->data->buf[0].p = usb_hw_ram_alloc(EP_MAX_SIZE);
		pas->data->buf[0].state = BufFree;
		pas->data->buf[1].p = usb_hw_ram_alloc(EP_MAX_SIZE);
		pas->data->buf[1].state = BufFree;
		pas->data->buf_queue = 0;
	}
}

USB_ENUM_HANDLER(&usb_audio2_enumeration);

// Disable HID on USB suspend

static void usb_audio2_susp()
{
	data.active = 0;
}

USB_SUSP_HANDLER(&usb_audio2_susp);

// Enable endpoints on Set Configuration

static void usb_audio2_config(uint8_t config)
{
	UNUSED(config);
	LIST_ITERATE(usb_audio2_streaming, usb_audio2_streaming_t, pas) {
		if (pas->type == UsbAudio2In) {
			usb_hw_ep_in_nak(pas->data->epnum);
			usb_hw_ep_out_nak(pas->data->fbepnum);
		} else {
			USB_TODO();
		}
	}
	data.active = 1;
}

USB_CONFIG_HANDLER(&usb_audio2_config);

// Configuration and report descriptor

USB_DESC_STRING_LIST();
USB_DESC_STRING(desc_if) = {0, LANG_EN_US, "USB Audio Class 2"};

static void usb_audio2_ac_iface_setup(usb_setup_t pkt, usb_desc_t *pdesc);
static void usb_audio2_as_iface_setup(usb_setup_t pkt, usb_desc_t *pdesc);

typedef struct PACKED {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
	uint8_t bRefresh;
	uint8_t bSynchAddress;
} desc_ep_sync_t;

void usb_desc_add_endpoint_sync(usb_desc_t *pdesc, uint8_t bEndpointAddress,
				uint8_t bmAttributes, uint16_t wMaxPacketSize,
				uint8_t bInterval, uint8_t bRefresh, uint8_t bSyncAddress)
{
	static const uint8_t bLength = 9u;
	const desc_ep_sync_t desc = {
		bLength, DESC_ENDPOINT, bEndpointAddress, bmAttributes, wMaxPacketSize,
		bInterval, bRefresh, bSyncAddress
	};
	usb_desc_add(pdesc, &desc, bLength);
}

static void usb_audio2_desc_out(usb_desc_t *pdesc, usb_audio2_streaming_t *pas)
{
	// Alternate setting 0, zero-bandwidth
	uint32_t iface = usb_desc_add_interface(pdesc, 0u, 0u, AUDIO, AUDIOSTREAMING,
						IP_VERSION_02_00, USB_DESC_STRING_INDEX(desc_if));
	usb_ep0_iface_register(iface, &usb_audio2_as_iface_setup);

	for (uint8_t mode = 1;; mode++) {
		// Class-specific AS interface descriptor
		uint8_t buf[pdesc->size];
		uint8_t bLength = pas->desc(mode, buf, pdesc->size);
		if (bLength == 0)
			break;
		if (pas->type == UsbAudio2Out) {
			USB_TODO();
			break;
		}

		// Alternate setting, operational
		usb_desc_add_interface(pdesc, mode, 2u, AUDIO, AUDIOSTREAMING,
				       IP_VERSION_02_00, USB_DESC_STRING_INDEX(desc_if));
		// Class-specific AS interface descriptor
		usb_desc_add(pdesc, &buf[0], bLength);
		// Data stream endpoint descriptor
		usb_desc_add_endpoint(pdesc, EP_DIR_OUT | pas->data->epnum,
				      EP_ISOCHRONOUS | EP_ISO_ASYNC | EP_ISO_DATA,
				      EP_MAX_SIZE, 1u);
		usb_desc_add(pdesc, &desc_ep[0], desc_ep[0].bLength);
		// Feedback endpoint descriptor
		usb_desc_add_endpoint(pdesc, EP_DIR_IN | pas->data->fbepnum,
				      EP_ISOCHRONOUS | EP_ISO_NONE | EP_ISO_FEEDBACK,
				      EP_MAX_SIZE, 1u);
	}
}

static void usb_audio2_desc_config(usb_desc_t *pdesc)
{
	// Interface association descriptor
	usb_desc_add_interface_association(pdesc, 1u + LIST_SIZE(usb_audio2_streaming),
					   AUDIO_FUNCTION, FUNCTION_SUBCLASS_UNDEFINED,
					   AF_VERSION_02_00, USB_DESC_STRING_INDEX(desc_if));

	// Audio control interface descriptor
	data.ac.iface = usb_desc_add_interface(pdesc, 0u, 0u, AUDIO, AUDIOCONTROL, IP_VERSION_02_00,
					       USB_DESC_STRING_INDEX(desc_if));
	usb_ep0_iface_register(data.ac.iface, &usb_audio2_ac_iface_setup);

	// Class-specific AudioControl interface descriptor and audio function descriptors
	desc_ac_t dac = desc_ac;
	uint8_t buf[pdesc->size];
	LIST_ITERATE(usb_audio2_entity, usb_audio2_entity_t, paf) {
#if DEBUG >= 5
		if (!paf->desc)
			USB_ERROR();
#endif
		uint8_t bLength = (*paf->desc)(buf + dac.wTotalLength, pdesc->size - dac.wTotalLength);
		dac.wTotalLength += bLength;
	}
	usb_desc_add(pdesc, &dac, dac.bLength);
	usb_desc_add(pdesc, &buf[0], dac.wTotalLength);

	LIST_ITERATE(usb_audio2_streaming, usb_audio2_streaming_t, pas) {
		if (pas->type == UsbAudio2In)
			usb_audio2_desc_out(pdesc, pas);
		else if (pas->type == UsbAudio2Out)
			USB_TODO();
#if DEBUG >= 5
		else
			USB_ERROR();
#endif
	}
}

USB_DESC_CONFIG_HANDLER(usb_audio2_desc_config);

// Interface control transfer

static uint32_t usb_audio2_ac_iface_data(usb_setup_t pkt, const void *p, uint32_t size)
{
	if (pkt.bEntityID == 0) {
		// Addressing interface
		USB_TODO();
		return -1;
	}

	usb_audio2_entity_t *pae = &LIST_AT(usb_audio2_entity, pkt.bEntityID - 1);
	if (!pae->set)
		return -1;
	return (*pae->set)(p, size, pkt.bRequest, pkt.bType, pkt.bIndex, pkt.wLength);
}

static void usb_audio2_ac_iface_setup(usb_setup_t pkt, usb_desc_t *pdesc)
{
	if (pkt.bEntityID == 0) {
		// Addressing interface
		USB_TODO();
		pdesc->size = -1;
		return;
	}

	usb_audio2_entity_t *pae = &LIST_AT(usb_audio2_entity, pkt.bEntityID - 1);
	switch (pkt.bmRequestType & USB_SETUP_TYPE_DIR_Msk) {
	case USB_SETUP_TYPE_DIR_D2H:
		if (pae->get)
			pdesc->size = (*pae->get)(pdesc->p, pdesc->size, pkt.bRequest,
						  pkt.bType, pkt.bIndex, pkt.wLength);
		else
			pdesc->size = -1;
		return;
	case USB_SETUP_TYPE_DIR_H2D:
		if (pkt.wLength) {
			usb_ep0_status_register(pkt, &usb_audio2_ac_iface_data);
			return;
		}
	}
	USB_TODO();
	pdesc->size = -1;
}

static uint32_t usb_audio2_as_set_mode(usb_setup_t pkt, usb_audio2_streaming_t *pas)
{
	uint8_t mode = pkt.wValue;
	if (!pas->set_mode)
		return -1;
	uint32_t bytes = (*pas->set_mode)(mode);
#if DEBUG >= 5
	printf(ESC_DEBUG "%lu\tusb_audio2: " ESC_WRITE "Set mode " ESC_DATA "%u"
	       ESC_DEBUG ", data rate " ESC_DATA "%lu bps\n",
	       systick_cnt(), mode, bytes * 8);
#endif
	if (bytes == (uint32_t)-1)
		return -1;
	return 0;
}

static void usb_audio2_as_iface_setup(usb_setup_t pkt, usb_desc_t *pdesc)
{
	usb_audio2_streaming_t *pas = &LIST_AT(usb_audio2_streaming, pkt.bID - data.ac.iface - 1);
	switch (pkt.bmRequestType & USB_SETUP_TYPE_DIR_Msk) {
	case USB_SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case USB_SETUP_SET_INTERFACE:
			pdesc->size = usb_audio2_as_set_mode(pkt, pas);
			return;
		}
		break;
	}
	USB_TODO();
	pdesc->size = -1;
}
