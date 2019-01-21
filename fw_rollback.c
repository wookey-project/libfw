#include "libfw.h"
#include "libflash.h"
#include "fw_storage.h"
#include "fw_mode.h"
#include "api/syscall.h"
#include "api/print.h"
#include "api/string.h"
#include "shr.h"


/*
 * The firmware version is structured on a uint32_t typed field.
 * It uses the following structure:
 *
 *       31         24        16           8            0
 *       -----------------------------------------------
 *       |  Major   | Middle   | patchset  |  devstage  |
 *       -----------------------------------------------
 *
 *  - sub (alpha, beta) for pre-release.
 *  final release of a given version should have its 'sub' value set
 *  with 0xff
 *  - patchset for patch management (security or functionality patchs, no evolution
 *  - Middle for minor evolution, compatible with previous releases of the same major
 *    version.
 *  - Major for major update, which may break compatibility with previous releases.
 *    The way the device is managed varies and the device content may be lost between
 *    major updates.
 *
 * Each field can go up to 255.
 * The version comparison is made using a direct uint32_t comparison.
 *
 */

#define VERSION_MAJOR_Pos 24
#define VERSION_MAJOR_Msk 0xff << VERSION_MAJOR_Pos

#define VERSION_MIDDLE_Pos 16
#define VERSION_MIDDLE_Msk 0xff << VERSION_MIDDLE_Pos

#define VERSION_PATCH_Pos 8
#define VERSION_PATCH_Msk 0xff << VERSION_PATCH_Pos

#define VERSION_DEV_Pos 0
#define VERSION_DEV_Msk 0xff << VERSION_DEV_Pos

int fw_version_compare(uint32_t version1, uint32_t version2)
{
    if (version1 > version2) {
        return -1;
    }
    if (version2 > version1) {
        return 1;
    }
    return 0;
}

uint32_t fw_get_current_version(firmware_version_field_t field)
{
    uint8_t ret;
    int desc = 0;
    t_firmware_state * fw = 0;
    uint32_t field_value = 0;
    shr_vars_t *shr_header;
    if (is_in_flip_mode()) {
        shr_header = (shr_vars_t*)FLASH_FLIP_SHR_ADDR;
    }
    if (is_in_flop_mode()) {
        shr_header = (shr_vars_t*)FLASH_FLOP_SHR_ADDR;
    }


#ifdef CONFIG_WOOKEY
    /* mapping device */
    if (is_in_flip_mode()) {
        desc = flash_get_descriptor(FLIP_SHR);
    }
    if (is_in_flop_mode()) {
        desc = flash_get_descriptor(FLOP_SHR);
    }
    ret = sys_cfg(CFG_DEV_MAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to map flash device\n");
        return 0;
    }

    if (is_in_flip_mode()) {
        fw = &(shr_header->fw);
    }
    if (is_in_flop_mode()) {
        fw = &(shr_header->fw);
    }

    /* get back current fw info (read only) - no flash unlock */
    uint32_t version = fw->fw_sig.version;

    /* unmap device */
    ret = sys_cfg(CFG_DEV_UNMAP, desc);
    if (ret != SYS_E_DONE) {
        printf("enable to map flash device\n");
        return 0;
    }

    /*return the field */
    switch (field) {
        case FW_VERSION_FIELD_MAJOR:
            field_value = (version & VERSION_MAJOR_Msk) >> VERSION_MAJOR_Pos;
            break;
        case FW_VERSION_FIELD_MIDDLE:
            field_value = (version & VERSION_MIDDLE_Msk) >> VERSION_MIDDLE_Pos;
            break;
        case FW_VERSION_FIELD_PATCH:
            field_value = (version & VERSION_PATCH_Msk) >> VERSION_PATCH_Pos;
            break;
        case FW_VERSION_FIELD_DEV:
            field_value = (version & VERSION_DEV_Msk) >> VERSION_DEV_Pos;
            break;
        case FW_VERSION_FIELD_ALL:
            field_value = version;
            break;
        default:
            printf("invalid field type!\n");
            break;
    }
#endif
    return field_value;

}

/* return true if new firmware version is smaller than current one */
bool fw_is_rollback(firmware_header_t *header)
{
    uint32_t current_version = fw_get_current_version(FW_VERSION_FIELD_ALL);
     /* we consider that a rollback means that the new version is
     * smaller or equel to the current one.
     * Equal versions generate rollback alert */
    if (fw_version_compare(current_version, header->version) <= 0) {
        return true;
    }
    return false;
}

