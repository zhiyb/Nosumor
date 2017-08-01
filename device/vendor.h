#ifndef VENDOR_H
#define VENDOR_H

#include "usb/hid/vendor/usb_hid_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

void vendor_process(hid_t *hid, vendor_report_t *rp);

#ifdef __cplusplus
}
#endif

#endif // VENDOR_H
