#include "api/libfw.h"
#include "api/types.h"
#include "autoconf.h"
#include "fw_mode.h"
#include "shr.h"
#include "libflash.h"
#include "libhash.h"
#include "libcryp.h"
#include "api/syscall.h"
#include "api/print.h"
#include "api/string.h"
#include "fw_storage.h"

/* clear the target DFU header (flip when in flop mode, flop when in flip mode */
uint8_t clear_other_header(void)
{
    uint8_t ret;
    int desc;
    t_firmware_state * fw = 0;
    uint8_t ok = 0;
    shr_vars_t *shr_header = 0;
    if (is_in_flip_mode()) {
        shr_header = (shr_vars_t*)FLASH_FLOP_SHR_ADDR;
    }
    if (is_in_flop_mode()) {
        shr_header = (shr_vars_t*)FLASH_FLIP_SHR_ADDR;
    }

    desc = flash_get_descriptor(CTRL2);
    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("unable to map flash-ctrl device\n");
        ok = 1;
        goto initial_err;
    }

    if (is_in_flip_mode()) {
        desc = flash_get_descriptor(FLOP_SHR);
    }
    if (is_in_flop_mode()) {
        desc = flash_get_descriptor(FLIP_SHR);
    }
    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("unable to map flip-shr device, rollback\n");
        ok = 1;
        goto middle_err;
    }


#if LIBFW_DEBUG
    printf("shr_header is stored in address %x\n", shr_header);
    printf("shr_header size is %x\n", sizeof(shr_vars_t));
#endif

    uint8_t buff[sizeof(t_firmware_state)] = { 0xff };

    flash_unlock();

    fw = &(shr_header->fw);
    /* flip and flop are *not* on the same sector */
    if (is_in_flip_mode()) {
#if LIBFW_DEBUG
        printf("clearing FLOP header at @ %x\n", &shr_header->fw);
#endif
    }
    if (is_in_flop_mode()) {
#if LIBFW_DEBUG
        printf("clearing FLIP header at @ %x\n", &shr_header->fw);
#endif
    }
    fw_storage_write_buffer((physaddr_t)fw, (uint32_t*)buff, sizeof(shr_vars_t));

    flash_lock();

    if (is_in_flip_mode()) {
        desc = flash_get_descriptor(FLOP_SHR);
    }
    if (is_in_flop_mode()) {
        desc = flash_get_descriptor(FLIP_SHR);
    }

    ret = sys_cfg(CFG_DEV_UNMAP, desc);
    if (ret != SYS_E_DONE) {
        printf("unable to map flip-shr device\n");
        return 1;
    }

middle_err:

    desc = flash_get_descriptor(CTRL2);
    ret = sys_cfg(CFG_DEV_UNMAP, desc);
    if (ret != SYS_E_DONE) {
        printf("unable to map flash-ctrl device\n");
        return 1;
    }

initial_err:
    return ok;
}

uint8_t set_fw_header(firmware_header_t *dfu_header, uint8_t *sig)
{
    uint8_t ret;
    int desc;
    t_firmware_state * fw = 0;
    uint8_t ok = 0;
    shr_vars_t *shr_header = 0;
    if (is_in_flip_mode()) {
        shr_header = (shr_vars_t*)FLASH_FLOP_SHR_ADDR;
    }
    if (is_in_flop_mode()) {
        shr_header = (shr_vars_t*)FLASH_FLIP_SHR_ADDR;
    }


    /*unmap hash if mapped */
    hash_unmap();
    /* map SHR */
    desc = flash_get_descriptor(CTRL2);
    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("unable to map flash-ctrl device\n");
        ok = 1;
        goto initial_err;
    }
    if (is_in_flip_mode()) {
        desc = flash_get_descriptor(FLOP_SHR);
    }
    if (is_in_flop_mode()) {
        desc = flash_get_descriptor(FLIP_SHR);
    }
    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("unable to map flip-shr device\n");
        ok = 1;
        goto middle_err;
    }

    fw = &(shr_header->fw);

    if (!fw) {
        printf("Error! unable to detect fw mode !\n");
        ok = 1;
        goto final_err;
    }
    uint32_t crc;
    t_firmware_state tmp_fw = { 0 };

    /* TODO: fw should be written in 2 times (in RAM, to set the CRC32, and
     * written to flash in atomic mode */
    tmp_fw.version = dfu_header->version;
    tmp_fw.siglen = dfu_header->siglen;
    memcpy((void*)tmp_fw.sig, sig, tmp_fw.siglen);
    tmp_fw.bootable = FW_BOOTABLE;

    /* set the header CRC32 */
    crc = crc32((uint8_t*)&tmp_fw, sizeof(t_firmware_state) - sizeof(uint32_t), 0xffffffff);
    tmp_fw.crc32 = crc;
    /* writing the structure, without the boot flag */
    tmp_fw.bootable = FW_NOT_BOOTABLE;

    fw_storage_write_buffer((physaddr_t)fw, (uint32_t*)&tmp_fw, sizeof(t_firmware_state));
    /* finishing with boot flag (atomic) */
    tmp_fw.bootable = FW_BOOTABLE;
    fw_storage_write_buffer((physaddr_t)&(fw->bootable), (uint32_t*)&(tmp_fw.bootable), sizeof(uint32_t));


    /* unmapping and rollback management */
final_err:

    if (is_in_flip_mode()) {
        desc = flash_get_descriptor(FLOP_SHR);
    }
    if (is_in_flop_mode()) {
        desc = flash_get_descriptor(FLIP_SHR);
    }

    ret = sys_cfg(CFG_DEV_UNMAP, desc);
    if (ret != SYS_E_DONE) {
        printf("unable to map flip-shr device\n");
        return ok;
    }

middle_err:

    desc = flash_get_descriptor(CTRL2);
    ret = sys_cfg(CFG_DEV_UNMAP, desc);
    if (ret != SYS_E_DONE) {
        printf("unable to map flash-ctrl device\n");
        return ok;
    }

initial_err:

    return ok;
}
