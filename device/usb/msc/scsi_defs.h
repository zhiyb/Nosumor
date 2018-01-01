#ifndef SCSI_DEFS_H
#define SCSI_DEFS_H

#include "scsi_defs_ops.h"
#include "scsi_defs_cmds.h"
#include "scsi_defs_sense.h"

// Qualifier(011b), Type(1fh): No peripheral device
#define PERIPHERAL_TYPE	0x7f

#endif // SCSI_DEFS_H
