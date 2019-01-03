#include "fw_storage.h"

uint8_t firmware_early_init(void)
{
    return fw_storage_early_init();
}

uint8_t firmware_init(void)
{
    return fw_storage_init();
}
