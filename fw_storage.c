#include "autoconf.h"
#include "api/libfw.h"
#include "api/types.h"
#include "api/print.h"
#include "libflash.h"
#include "fw_storage.h"
#include "api/syscall.h"

uint8_t fw_storage_early_init(void)
{
    int ret = 0;

    if (is_in_flip_mode()) {
        t_device_mapping devmap = {
#ifdef CONFIG_WOOKEY
            .map_flip_shr = 0,
            .map_flip = 0,
            .map_flop_shr = 0,
            .map_flop = 1,
#else
# if CONFIG_USR_DRV_FLASH_DUAL_BANK
            .map_mem_bank1 = 0,
            .map_mem_bank2 = 1,
# else
            .map_mem = 1,
# endif
#endif
            .map_ctrl = 1,
            .map_system = 0,
            .map_otp = 0,
            .map_opt_bank1 = 0,
#if CONFIG_USR_DRV_LASH_DUAL_BANK
            .map_opt_bank2 = 1,
#endif
        };
        ret = flash_device_early_init(&devmap);
        // mapping flop
    } else if (is_in_flop_mode()) {
        // mapping flip
        t_device_mapping devmap = {
#ifdef CONFIG_WOOKEY
            .map_flip_shr = 0,
            .map_flip = 1,
            .map_flop_shr = 0,
            .map_flop = 0,
#else
# if CONFIG_USR_DRV_FLASH_DUAL_BANK
            .map_mem_bank1 = 1,
            .map_mem_bank2 = 0,
# else
            .map_mem = 1,
# endif
#endif
            .map_ctrl = 1,
            .map_system = 0,
            .map_otp = 0,
            .map_opt_bank1 = 1,
#if CONFIG_USR_DRV_LASH_DUAL_BANK
            .map_opt_bank2 = 0,
#endif
        };
        ret = flash_device_early_init(&devmap);

    } else {
        printf("unknown mode !\n");
        ret = 1;
    }
    if (ret != 0) {
        goto err;
    }
    return 0;

err:
    return 1;
}


uint8_t fw_storage_init(void)
{
    uint8_t ret;
    int desc = flash_get_descriptor(FLOP);
    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to map flop device\n");
    }
    desc = flash_get_descriptor(CTRL);
    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to map flash-ctrl device\n");
    }
    return 0;
}
