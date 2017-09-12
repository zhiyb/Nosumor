#include <malloc.h>
#include "../usb_desc.h"
#include "../usb_setup.h"
#include "../usb_ep.h"
#include "../usb_structs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_desc.h"
#include "usb_audio2_entities.h"

/* USB packet decodeing */

void usb_audio2_get(usb_t *usb, usb_audio_t *data, uint32_t ep, setup_t pkt)
{
	desc_t desc = {0, 0};
	for (usb_audio_entity_t **p = &data->entities; *p; p = &(*p)->next)
		if ((*p)->id == pkt.bEntityID) {
			if (!(*p)->get) {
				dbgbkpt();
				break;
			}
			desc = (*p)->get(data, *p, pkt);
			break;
		}
	if (desc.size == 0) {
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	} else {
		if (desc.size > pkt.wLength)
			desc.size = pkt.wLength;
		usb_ep_in_transfer(usb->base, ep, desc.p, desc.size);
	}
}

void usb_audio2_set(usb_t *usb, usb_audio_t *data, uint32_t ep, setup_t pkt)
{
	int ret = 0;
	for (usb_audio_entity_t **p = &data->entities; *p; p = &(*p)->next)
		if ((*p)->id == pkt.bEntityID) {
			if (!(*p)->set) {
				dbgbkpt();
				break;
			}
			ret = (*p)->set(data, *p, pkt);
			break;
		}
	if (ret)
		usb_ep_in_transfer(usb->base, ep, 0, 0);
	else
		usb_ep_in_stall(usb->base, ep);
}

usb_audio_entity_t *usb_audio2_register_entity(usb_audio_t *audio, const uint8_t id, const void *data)
{
	// Allocate entitry
	usb_audio_entity_t *entity = malloc(sizeof(usb_audio_entity_t));
	if (!entity)
		panic();
	entity->id = id;
	entity->data = data;
	entity->next = 0;
	// Add to list
	usb_audio_entity_t **p;
	for (p = &audio->entities; *p; p = &(*p)->next);
	*p = entity;
	return entity;
}
