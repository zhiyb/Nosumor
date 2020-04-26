#ifndef DEFINES_H
#define DEFINES_H

#define USB_VID			0x0483
#define USB_PID			0x5750
#define USB_RELEASE		SWVER
#define USB_MANUFACTURER	"zhiyb"
#define USB_PRODUCT_NAME	"USB Mix"

// 25 bytes space (null-terminated) required
void uid_str(char *s);

#endif // DEFINES_H
