#include "fw_storage.h"

uint8_t firmware_early_init(t_device_mapping *devmap)
{
    return fw_storage_early_init(devmap);
}

uint8_t firmware_init(void)
{
    return fw_storage_init();
}
