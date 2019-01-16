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
 * 31     28         21         14          7            0
 * ------------------------------------------------------
 * |Epoch |  Major   | Middle   |   Minor   |   Update   |
 * ------------------------------------------------------
 *
 *  Update, minor, middle and major fields are unsigned integer values,
 *  up to 127.
 *  Epoch is an unsigned value up to 7.
 *
 *  Using epoch allows to change the versioning if needed but is
 *  not required and can be leaved to 0 (default epoch).
 *
 *  As fields are encoded from the less impacting (update) to the
 *  more impacting (epoch) one, it is possible to compare the
 *  versions using the uint32_t types.
 *  Yet each field can be read separatly if needed
 */

#define VERSION_EPOCH_Pos 28
#define VERSION_EPOCH_Msk 0x3 << VERSION_EPOCH_Pos

#define VERSION_MAJOR_Pos 21
#define VERSION_MAJOR_Msk 0x7f << VERSION_MAJOR_Pos

#define VERSION_MIDDLE_Pos 14
#define VERSION_MIDDLE_Msk 0x7f << VERSION_MIDDLE_Pos

#define VERSION_MINOR_Pos 7
#define VERSION_MINOR_Msk 0x7f << VERSION_MINOR_Pos

#define VERSION_UPDATE_Pos 0
#define VERSION_UPDATE_Msk 0x7f << VERSION_MINOR_Pos

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
    int desc;
    t_firmware_state * fw = 0;
    uint32_t field_value = 0;
    shr_vars_t *shr_header = (shr_vars_t*)FLASH_SHR_ADDR;

#ifdef CONFIG_WOOKEY
    if (!flash_is_device_registered(FLIP_SHR)) {
#else
    if (!flash_is_device_registered(BANK1)) {
#endif
        /* mapping device */
#ifdef CONFIG_WOOKEY
        desc = flash_get_descriptor(FLIP_SHR);
#else
        desc = flash_get_descriptor(BANK1);
#endif
        ret = sys_cfg(CFG_DEV_MAP, desc);
        if (ret != SYS_E_DONE) {
            printf("enable to map flash device\n");
            return 0;
        }

        if (is_in_flip_mode()) {
            fw = &(shr_header->flip);
        }
        if (is_in_flop_mode()) {
            fw = &(shr_header->flop);
        }

        /* get back current fw info (read only) - no flash unlock */
        uint32_t version = fw->version;

        /* unmap device */
        ret = sys_cfg(CFG_DEV_UNMAP, desc);
        if (ret != SYS_E_DONE) {
            printf("enable to map flash device\n");
            return 0;
        }

        /*return the field */
        switch (field) {
            case FW_VERSION_FIELD_EPOCH:
                field_value = (version & VERSION_EPOCH_Msk) >> VERSION_EPOCH_Pos;
                break;
            case FW_VERSION_FIELD_MAJOR:
                field_value = (version & VERSION_MAJOR_Msk) >> VERSION_MAJOR_Pos;
                break;
            case FW_VERSION_FIELD_MIDDLE:
                field_value = (version & VERSION_MIDDLE_Msk) >> VERSION_MIDDLE_Pos;
                break;
            case FW_VERSION_FIELD_MINOR:
                field_value = (version & VERSION_MINOR_Msk) >> VERSION_MINOR_Pos;
                break;
            case FW_VERSION_FIELD_UPDATE:
                field_value = (version & VERSION_UPDATE_Msk) >> VERSION_UPDATE_Pos;
                break;
            case FW_VERSION_FIELD_ALL:
                field_value = version;
                break;
            default:
                printf("invalid field type!\n");
                break;
        }
    } else {
        printf("unable to map headers\n");
    }
    return field_value;

}

/* return true if new firmware version is smaller than current one */
bool fw_is_rollback(firmware_header_t *header)
{
    uint32_t current_version = fw_get_current_version(FW_VERSION_FIELD_ALL);
    /* we consider that a rollback means that the new version is strictly
     * smaller than the current one. Equal versions don't generate rollback
     * alert */
    if (fw_version_compare(current_version, header->version) < 0) {
        return true;
    }
    return false;
}


