#ifndef USB_AUDIO2_H
#define USB_AUDIO2_H

#include <stdint.h>
#include <list.h>

// Audio function descriptor and control
typedef const struct usb_audio2_entity_t {
	uint8_t (* const desc)(void *p, uint32_t size);
	uint32_t (* const get)(void *p, uint32_t size, uint8_t request,
			       uint8_t cs, uint8_t cn, uint16_t length);
	uint32_t (* const set)(const void *p, uint32_t size, uint8_t request,
			       uint8_t cs, uint8_t cn, uint16_t length);
} usb_audio2_entity_t;

// USB_AUDIO2_FUNCTION(name(optional)) = (usb_audio2_function_t){};
#define USB_AUDIO2_ENTITY(...)		LIST_ITEM(usb_audio2_entity, usb_audio2_entity_t, ## __VA_ARGS__)
#define USB_AUDIO2_ENTITY_LIST()	LIST(usb_audio2_entity, usb_audio2_entity_t)
#define USB_AUDIO2_ENTITY_INDEX(p)	LIST_INDEX(usb_audio2_entity, p)
#define USB_AUDIO2_ENTITY_ID(p)		(USB_AUDIO2_ENTITY_INDEX(p) + 1)

// Audio streaming interface
typedef enum {UsbAudio2In = 0, UsbAudio2Out} usb_audio2_streaming_type_t;

typedef enum {BufFree = 0, BufActive} usb_audio2_buf_state_t;

typedef struct usb_audio2_data_t {
	uint32_t epnum, fbepnum;
	struct buf_t {
		volatile usb_audio2_buf_state_t state;
		void *p;
	} buf[2];
	volatile uint32_t buf_queue;
} usb_audio2_data_t;

typedef const struct usb_audio2_streaming_t {
	usb_audio2_streaming_type_t type;
	uint8_t (* const terminal)(const struct usb_audio2_streaming_t *pas);
	uint8_t (* const desc)(uint8_t mode, void *p, uint32_t size);
	uint32_t (* const set_mode)(uint8_t mode);
	usb_audio2_data_t *data;
} usb_audio2_streaming_t;

// USB_AUDIO2_STREAMING(name(optional)) = (usb_audio2_function_t){};
#define USB_AUDIO2_STREAMING(...)	LIST_ITEM(usb_audio2_streaming, usb_audio2_streaming_t, ## __VA_ARGS__)
#define USB_AUDIO2_STREAMING_LIST()	LIST(usb_audio2_streaming, usb_audio2_streaming_t)
#define USB_AUDIO2_STREAMING_INDEX(p)	LIST_INDEX(usb_audio2_streaming, p)
#define USB_AUDIO2_STREAMING_ID(p)	(USB_AUDIO2_STREAMING_INDEX(p))

#endif // USB_AUDIO2_H
