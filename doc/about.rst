About the libfirmware usages
----------------------------

Run mode usage
""""""""""""""

When using the libfirmware as a run state detection backend, it can be used by any application to detect in which run mode and bank it is being executed. This allows executable that are compiled for multiple banks and execution mode to react differently depending on the current bank and mode.

The libfirmware supports the following, device-generic banks:

   * Flip bank
   * Flop bank

These banks are generic, dual-bank based resilient firmware images.

The libfirmware supports the following, device-generic run mode:

   * FW mode (aka. nominal mode)
   * DFU mode (aka. upgrade mode)

We consider these two run mode also as generic modes.

The libfirmware is based on ldscript variables to detect bank and mode. As a consequence, it is up to the linker to define the following variables::

   __is_flip
   __is_flop
   __is_fw
   __is_dfu

Each binary application should then have a *4-uplet* with specific addresses in its ldscript.

These variables are boolean variables, but as they are ldscript variables, only their addresses can be used. As a consequence, an address of 0xf0 (i.e. 240) is considered as *True*, any other address is considered as *False*.

These variables must be set at the begining of the ldscript file, out of any SECTIONS block. A typical definition would be::

   __is_flip = 240;
   __is_flop = 0;
   __is_fw   = 240;
   __is_dfu  = 0;

This definition define the application as being executed in flip mode, in nominal (i.e. FW) mode.

.. hint::
   It is possible to use the libfirmware without differenciate run mode or with a single bank. As variables are boolean, they can be always defined as false


Firmware image manipulation
"""""""""""""""""""""""""""

Manipulating firmware images should be the job of a dedicated task. This task
**must** hold the TSK_UPGRADE permission to use this part of the libfirmware.

.. hint::
   Tasks that only manipulate run mode will have this part of the libfirmware removed from the generated binary at link time, as these functions are not called

Manipulating firmware is more complex than only handling raw binary file. In Wookey (and as a consequence libfirmware) firmware images host header informations which contains various data, including cryptographic signatures, version, and various meta-data.

The libfirmware provides helper functions to manipulate such header, including parsing, version checking and so on.

The libfirmware also handle the firmware storage backend, including the firmware image update and the firmware bootloader metainformation header update.

.. warning::
   All the header related API is specific to the way firmware headers and bootloader header update are handled. This part of the libfirmware can be ported from one hardware to another, but is sticked to the way Wookey is handling its upgrade

