#include <malloc.h>
#include "../usb_desc.h"
#include "usb_audio2_defs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_entities.h"
#include "usb_audio2_desc.h"

void usb_audio2_register_it(usb_audio_t *audio, const uint8_t id, const audio_it_t *data,
			    uint16_t wTerminalType, uint8_t bAssocTerminal, uint8_t bCSourceID,
			    uint8_t bNrChannels, uint32_t bmChannelConfig, uint8_t iChannelNames,
			    uint16_t bmControls, uint8_t iTerminal)
{
	// Register entitry
	usb_audio_entity_t *entity = usb_audio2_register_entity(audio, id, data);
	entity->get = 0;
	entity->set = 0;
	// Add descriptor
	const desc_it_t desc = {
		17u, CS_INTERFACE, INPUT_TERMINAL, id, wTerminalType, bAssocTerminal,
		bCSourceID, bNrChannels, bmChannelConfig, iChannelNames, bmControls, iTerminal
	};
	void *pd = malloc(desc.bLength);
	if (!pd)
		panic();
	memcpy(pd, &desc, desc.bLength);
	entity->desc = pd;
}

void usb_audio2_register_ot(usb_audio_t *audio, const uint8_t id, const audio_ot_t *data,
			    uint16_t wTerminalType, uint8_t bAssocTerminal,
			    uint8_t bSourceID, uint8_t bCSourceID,
			    uint16_t bmControls, uint8_t iTerminal)
{
	// Register entitry
	usb_audio_entity_t *entity = usb_audio2_register_entity(audio, id, data);
	entity->get = 0;
	entity->set = 0;
	// Add descriptor
	const desc_ot_t desc = {
		12u, CS_INTERFACE, OUTPUT_TERMINAL, id, wTerminalType, bAssocTerminal,
		bSourceID, bCSourceID, bmControls, iTerminal
	};
	void *pd = malloc(desc.bLength);
	if (!pd)
		panic();
	memcpy(pd, &desc, desc.bLength);
	entity->desc = pd;
}
