#ifndef FW_STORAGE_H_
#define FW_STORAGE_H_

#include "api/types.h"
#include "libflash.h"

uint8_t fw_storage_early_init(t_device_mapping *devmap);

uint8_t fw_storage_init(void);

#endif/*!FW_STORAGE_H_*/
