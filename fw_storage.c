#include "autoconf.h"
#include "api/libfw.h"
#include "api/types.h"
#include "api/print.h"
#include "libflash.h"
#include "fw_storage.h"
#include "api/syscall.h"

#define FW_STORAGE_DEBUG 1

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
#if CONFIG_USR_DRV_FLASH_DUAL_BANK
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
#if CONFIG_USR_DRV_FLASH_DUAL_BANK
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

    return 0;
#if 0
    uint8_t ret;
    int desc;

    if (is_in_flip_mode()) {
        desc = flash_get_descriptor(FLOP);
        ret = sys_cfg(CFG_DEV_MAP, desc);
        if (ret != SYS_E_DONE) {
            printf("enable to map flop device\n");
        }
        desc = flash_get_descriptor(OPT_BANK2);
        ret = sys_cfg(CFG_DEV_MAP, desc);
        if (ret != SYS_E_DONE) {
            printf("enable to map flash-opt-bank2 device\n");
        }
    }
    if (is_in_flop_mode()) {
        desc = flash_get_descriptor(FLIP);
        ret = sys_cfg(CFG_DEV_MAP, desc);
        if (ret != SYS_E_DONE) {
            printf("enable to map flip device\n");
        }
        desc = flash_get_descriptor(OPT_BANK1);
        ret = sys_cfg(CFG_DEV_MAP, desc);
        if (ret != SYS_E_DONE) {
            printf("enable to map flash-opt-bank1 device\n");
        }
    }
    desc = flash_get_descriptor(CTRL);
    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to map flash-ctrl device\n");
    }
#endif

    return 0;
}

/* This function erase the other firmware (i.e. flip if in flop, flop if in
 * flip) flash sectors. The bootloader & SHR sectors are *not* erased
 * The code bellow is implemented for 2MB flash size. */
uint8_t fw_storage_erase_bank(void)
{
    uint8_t ret;
    int desc;

    /* mapping flash-ctrl */
    desc = flash_get_descriptor(CTRL);
    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to map flash-ctrl device\n");
        return 1;
    }
    
    /* unlocking flash */
    flash_unlock();

    if (is_in_flip_mode()) {
        /* erasing flop sectors */
# if (CONFIG_USR_DRV_FLASH_1M && CONFIG_USR_DRV_FLASH_DUAL_BANK) || CONFIG_USR_DRV_FLASH_2M
        flash_sector_erase(FLASH_SECTOR_17);
        flash_sector_erase(FLASH_SECTOR_18);
        flash_sector_erase(FLASH_SECTOR_19);
#endif
# if CONFIG_USR_DRV_FLASH_2M
        flash_sector_erase(FLASH_SECTOR_20);
        flash_sector_erase(FLASH_SECTOR_21);
        flash_sector_erase(FLASH_SECTOR_22);
        flash_sector_erase(FLASH_SECTOR_23);
#endif
    }
    if (is_in_flop_mode()) {
        /* erasing flop sectors */
        flash_sector_erase(FLASH_SECTOR_5);
        flash_sector_erase(FLASH_SECTOR_6);
        flash_sector_erase(FLASH_SECTOR_7);
# if (CONFIG_USR_DRV_FLASH_1M && !CONFIG_USR_DRV_FLASH_DUAL_BANK) || CONFIG_USR_DRV_FLASH_2M
        flash_sector_erase(FLASH_SECTOR_8);
        flash_sector_erase(FLASH_SECTOR_9);
        flash_sector_erase(FLASH_SECTOR_10);
        flash_sector_erase(FLASH_SECTOR_11);
#endif
    }

    /* lock flash CR */
    flash_lock();
    /* unmap flash-ctrl */
    ret = sys_cfg(CFG_DEV_UNMAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to unmap flash-ctrl device\n");
        return 1;
    }

    return 0;
}

uint8_t fw_storage_prepare_access(void)
{

    uint8_t ret;
    int desc;
    uint8_t  desc_id = 0;

    /* mapping flash-ctrl */
    desc = flash_get_descriptor(CTRL);

#if FW_STORAGE_DEBUG
    printf("mapping flash-ctrl (desc: %d)\n", desc);
#endif
    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to map flash-ctrl device\n");
        return 1;
    }

    if (is_in_flip_mode()) {
        desc_id = FLOP;

    } else if (is_in_flop_mode()) {
        desc_id = FLIP;
    } else {
        printf("neither in flip or flop mode !\n");
        return 1;
    }
    /* mounting flash memory area */
    desc = flash_get_descriptor(desc_id);
#if FW_STORAGE_DEBUG
    printf("mappint flash-ctrl (desc: %d)\n", desc);
#endif

    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to map flash-ctrl device\n");
        return 1;
    }

    /* unlocking flash */
    flash_unlock();
    return 0;
}

uint8_t fw_storage_finalize_access(void)
{
    uint8_t ret;
    int desc = 0;
    uint8_t  desc_id = 0;

    if (is_in_flip_mode()) {
        desc_id = FLOP;

    } else if (is_in_flop_mode()) {
        desc_id = FLIP;
    } else {
        printf("neither in flip or flop mode !\n");
        return 1;
    }

    desc = flash_get_descriptor(desc_id);
#if FW_STORAGE_DEBUG
    printf("unmapping flash-area (desc: %d)\n", desc);
#endif

    ret = sys_cfg(CFG_DEV_UNMAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to unmap flash memory device\n");
        return 1;
    }

#if FW_STORAGE_DEBUG
    printf("releasing flash memory (desc: %d)\n", desc);
#endif
    ret = sys_cfg(CFG_DEV_RELEASE, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to release flash memory device\n");
        return 1;
    }
    

    /* lock flash CR */
    flash_lock();
    /* unmap flash-ctrl */
    desc = flash_get_descriptor(CTRL);

#if FW_STORAGE_DEBUG
    printf("unmapping flash-ctrl (desc: %d)\n", desc);
#endif
    ret = sys_cfg(CFG_DEV_UNMAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to unmap flash-ctrl device\n");
        return 1;
    }

#if FW_STORAGE_DEBUG
    printf("releasing flash-ctrl (desc: %d)\n", desc);
#endif
    ret = sys_cfg(CFG_DEV_RELEASE, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to release flash-ctrl device\n");
        return 1;
    }

    return 0;
}

/*
 * Here we consider we write *words*. This means that the size must be
 * 4bytes multiple. Size is still in bytes.
 */
uint8_t fw_storage_write_buffer(physaddr_t dest, uint32_t *buffer, uint32_t size)
{
    if (is_in_flip_mode()) {
        /* sanitize */
        uint8_t sector = flash_select_sector(dest);
        if (sector < 11) {
            printf("destination not in the other bank !!!\n");
            return 1;
        }
    } else if (is_in_flop_mode()) {
        /* sanitize */
        uint8_t sector = flash_select_sector(dest);
        if (sector > 12) {
            printf("destination not in the other bank !!!\n");
            return 1;
        }
    } else {
        printf("neither in flip or flop mode !\n");
        return 1;
    }

    uint32_t *addr = (uint32_t *)dest;
    uint32_t *offset = buffer;
    uint32_t residue = 0;
    uint32_t aligned_size = size - (size % 4);
    if (size % 4) {
        /* size is not word-aligned ! a residue is needed */
        residue = size % 4;
    }
    for (uint32_t i = 0; i < (aligned_size / 4); ++i) {
        flash_program_word(addr, *offset);
        addr++;
        offset++;
    }
    /* if size is not 4 bytes aligned, finish with the up
     * to 3 bytes to write */
    if (residue) {
        uint8_t *u8_addr  = (uint8_t*)addr;
        uint8_t *u8_offset  = (uint8_t*)offset;
        for (uint32_t i = 0; i < residue; ++i) {
            /* FIXME: cast here is brainfuck. Should be made cleaner */
            flash_program_byte(u8_addr, *u8_offset);
            u8_addr++;
            u8_offset++;
        }
    }


    return 0;
}
