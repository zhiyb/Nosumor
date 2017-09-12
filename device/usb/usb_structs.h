#ifndef USB_STRUCT_H
#define USB_STRUCT_H

#include <stdint.h>
#include "../macros.h"
#include "usb_desc.h"
#include "usb_setup.h"

#define USB_EPIN_CNT	8
#define USB_EPOUT_CNT	8

#ifdef __cplusplus
extern "C" {
#endif

// Endpoint handlers
typedef struct epin_t {
	void *data;
	void (*init)(usb_t *usb, uint32_t ep);
	void (*halt)(usb_t *usb, uint32_t ep, int halt);
	void (*timeout)(usb_t *usb, uint32_t ep);
	void (*xfr_cplt)(usb_t *usb, uint32_t ep);
	int isoc_check;
} epin_t;

typedef struct epout_t {
	void *data;
	void (*init)(usb_t *usb, uint32_t ep);
	void (*halt)(usb_t *usb, uint32_t ep, int halt);
	void (*setup_cplt)(usb_t *usb, uint32_t ep);
	void (*xfr_cplt)(usb_t *usb, uint32_t ep);
	int isoc_check;
} epout_t;

// Interface handlers
typedef struct usb_if_t {
	// Linked list
	struct usb_if_t *next;
	// Interface ID
	uint32_t id;
	// Private data
	void *data;
	// Function handlers
	void (*config)(usb_t *usb, void *data);
	void (*enable)(usb_t *usb, void *data);
	void (*disable)(usb_t *usb, void *data);
	void (*setup_std)(usb_t *usb, void *data, uint32_t ep, setup_t pkt);
	void (*setup_class)(usb_t *usb, void *data, uint32_t ep, setup_t pkt);
} usb_if_t;

typedef enum {
	USB_Reset = 0, USB_LowSpeed, USB_FullSpeed, USB_HighSpeed,
} usb_speed_t;

typedef enum {
	USB_Self_Powered = 1, USB_Remote_Wakeup = 2,
} usb_status_t;	// bit-fields

// USB port data structure
typedef struct usb_t {
	USB_OTG_GlobalTypeDef *base;
	usb_speed_t speed;
	usb_status_t status;
	// USB RAM & FIFO allocation
	struct {
		uint32_t top, max;
		uint32_t fifo;
	};
	// Endpoint allocation
	uint32_t nepin, nepout;
	epin_t epin[USB_EPIN_CNT];
	epout_t epout[USB_EPOUT_CNT];
	// Descriptors
	struct {
		desc_t dev, dev_qua, config;
		desc_t lang;
		desc_string_list_t **string;
		uint32_t nstring;
	} desc;
	// Interfaces
	usb_if_t *usbif;
} usb_t;

#ifdef __cplusplus
}
#endif

#endif
