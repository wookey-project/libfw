#include "autoconf.h"
#include "api/libfw.h"
#include "api/types.h"
#include "autoconf.h"
#include "shr.h"
#include "libflash.h"
#include "libhash.h"
#include "libcryp.h"
#include "api/syscall.h"
#include "api/stdio.h"
#include "api/nostd.h"
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
        shr_header = (shr_vars_t*)CONFIG_USR_LIB_FIRMWARE_FLOP_BOOTINFO_ADDR;
    }
    if (is_in_flop_mode()) {
        shr_header = (shr_vars_t*)CONFIG_USR_LIB_FIRMWARE_FLIP_BOOTINFO_ADDR;
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

uint8_t set_fw_header(const firmware_header_t *dfu_header, const uint8_t *sig, const uint8_t *hash)
{
    uint8_t ret;
    int desc;
    t_firmware_state * fw = 0;
    uint8_t ok = 0;
    shr_vars_t *shr_header = 0;
    if (is_in_flip_mode()) {
        shr_header = (shr_vars_t*)CONFIG_USR_LIB_FIRMWARE_FLOP_BOOTINFO_ADDR;
    }
    if (is_in_flop_mode()) {
        shr_header = (shr_vars_t*)CONFIG_USR_LIB_FIRMWARE_FLOP_BOOTINFO_ADDR;
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
    t_firmware_signature tmp_fw;
    memset((uint8_t*)&tmp_fw, 0xff, sizeof(t_firmware_signature));
    uint32_t bootable = FW_BOOTABLE;

    /* TODO: fw should be written in 2 times (in RAM, to set the CRC32, and
     * written to flash in atomic mode */
    tmp_fw.magic = dfu_header->magic;
    tmp_fw.type = dfu_header->type;
    tmp_fw.version = dfu_header->version;
    tmp_fw.len = dfu_header->len;
    tmp_fw.siglen = dfu_header->siglen;
    tmp_fw.chunksize = dfu_header->chunksize;
    memcpy((void*)tmp_fw.sig, sig, tmp_fw.siglen);
    memcpy((void*)tmp_fw.hash, hash, SHA256_DIGEST_SIZE);
    //tmp_fw.bootable = FW_BOOTABLE;

    /* CRC32 field is not checked for CRC32. We use it as tmp buf to calculate
     * the CRC32 of the overall SHR sectors (2 sectors) which must contain, at
     * boot, only 0xfffff out of the signature header, as these sectors
     * have been erased */
    tmp_fw.crc32 = 0xffffffff;

    /* set the signature header CRC32 (with the CRC32 field set at 0xffffffff) */
    crc = crc32((uint8_t*)&tmp_fw, sizeof(t_firmware_signature) - SHA256_DIGEST_SIZE - EC_MAX_SIGLEN, 0xffffffff);

    crc = crc32((uint8_t*)tmp_fw.hash, SHA256_DIGEST_SIZE, crc);
    /* signature is not a part of the CRC, we use 0xff...ff instead */
    for (uint32_t i = 0; i < EC_MAX_SIGLEN; ++i) {
        crc = crc32((uint8_t*)&tmp_fw.crc32, sizeof(uint8_t), crc);
    }
    /* equivalent to calculcating CRC32 of 0xffff of 'fill' field of the header */
    for (uint32_t i = 0; i < (SHR_SECTOR_SIZE - sizeof(t_firmware_signature)); ++i) {
        crc = crc32((uint8_t*)&tmp_fw.crc32, sizeof(uint8_t), crc);
    }
    /* finishing with boot flag crc32 */
    crc = crc32((uint8_t*)&bootable, sizeof(uint32_t), crc);
    /* and the vfill residue of the 2nd sector (using crc32 as temp buffer
     * containing 0xffffffff content */
    for (uint32_t i = 0; i < (SHR_SECTOR_SIZE - sizeof(uint32_t)); ++i) {
        crc = crc32((uint8_t*)&tmp_fw.crc32, sizeof(uint8_t), crc);
    }
    /* update the crc32 field with the calculated CRC */
    tmp_fw.crc32 = crc;

    flash_unlock();

    printf("writing header singature :@%x\n", fw);
    fw_storage_write_buffer((physaddr_t)fw, (uint32_t*)&tmp_fw, sizeof(t_firmware_signature));
    printf("writing header bootflag :@%x\n", (uint32_t)&fw->bootable);
    fw_storage_write_buffer((physaddr_t)&fw->bootable, (uint32_t*)&bootable, sizeof(uint32_t));

    flash_lock();

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
