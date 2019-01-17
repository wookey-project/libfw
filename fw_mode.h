#ifndef FW_MODE_H_
#define FW_MODE_H_

/* FIXME: these valies should be read from the layout preprocessing */
#define FLASH_FLOP_ADDR 0x08120000
#define FLASH_FLIP_ADDR 0x08020000
#define FLASH_FLIP_SHR_ADDR  0x08008000
#define FLASH_FLOP_SHR_ADDR  0x08108000
#define FLASH_SIZE 0xe0000 // fw+dfu size (without SHR & bootloader)



#endif/*!FW_MODE_H_*/
