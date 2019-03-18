#include "api/libfw.h"
#include "fw_mode.h"

#include "api/types.h"
#include "api/stdio.h"
#include "api/nostd.h"

/* these variables are linker script symbols. This means that they
 * have *no value*. Their address stored in the symbol table is used
 * instead.
 */
extern int __is_flip;
extern int __is_flop;
extern int __is_fw;
extern int __is_dfu;

void dump_states(void)
{
    int a = __is_flip;
    printf("flip: %x\n", a);
    a = __is_flop;
    printf("flop: %x\n", a);
    a = __is_dfu;
    printf("dfu:  %x\n", a);
    a = __is_fw;
    printf("fw:   %x\n", a);
}


/*
 * FIXME: this function should use secbool instead of standard C boolean
 * to avoid fault injection.
 */
bool is_in_flip_mode(void)
{
    if (&__is_flip == (void*)0xf0) {
        return true;
    }
    return false;
}

/*
 * FIXME: this function should use secbool instead of standard C boolean
 * to avoid fault injection.
 */
bool is_in_flop_mode(void)
{
    if (&__is_flop == (void*)0xf0) {
        return true;
    }
    return false;
}

bool is_in_fw_mode(void)
{
    if (&__is_fw == (void*)0xf0) {
        return true;
    }
    return false;
}


/*
 * FIXME: this function should use secbool instead of standard C boolean
 * to avoid fault injection.
 */
bool is_in_dfu_mode(void)
{
    if (&__is_dfu == (void*)0xf0) {
        return true;
    }
    return false;
}

uint32_t firmware_get_flip_base_addr(void)
{
    return FLASH_FLIP_ADDR;
}

uint32_t firmware_get_flop_base_addr(void)
{
    return FLASH_FLOP_ADDR;
}

uint32_t firmware_get_flip_size(void)
{
    return FLASH_SIZE;
}

uint32_t firmware_get_flop_size(void)
{
    return FLASH_SIZE;
}
