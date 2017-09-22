#ifndef USB_MSC_DEFS_H
#define USB_MSC_DEFS_H

/* Specification Overview v1.4 */

// Interface class code
#define MASS_STORAGE	0x08

// 2 Subclass Codes
#define SCSI_NOT_REPORTED	0x00
#define RBC			0x01
#define MMC_5_ATAPI		0x02
#define UFI			0x04
#define SCSI_TRANSPARENT	0x06
#define LSD_FS			0x07
#define IEEE_1667		0x08
#define SUBCLASS_VENDOR		0xff

// 3 Protocol Codes
#define CBI_CC		0x00
#define CBI_NO_CC	0x01
#define BBB		0x50
#define UAS		0x62
#define PROTOCOL_VENDOR	0xff

// 4 Request Codes
#define ADSC	0x00
#define GET	0xfc
#define PUT	0xfd
#define GML	0xfe
#define BOMSR	0xff

// 5 Class Specific Descriptor Codes
#define PIPE_USAGE	0x24

/* Bulk-Only Transport v1.0 */

#endif // USB_MSC_DEFS_H
