/* \file libfw.h
 *
 * Copyright 2018 The wookey project team <wookey@ssi.gouv.fr>
 *   - Ryad     Benadjila
 *   - Arnauld  Michelizza
 *   - Mathieu  Renard
 *   - Philippe Thierry
 *   - Philippe Trebuchet
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 */
#ifndef LIBFW_H_
#define LIBFW_H_

#include "api/types.h"

/***********************************************************
 * basic check and signature functions
 **********************************************************/

/*
 * @brief Linux-compatible CRC32 implementation
 *
 * Return the CRC32 of the current buffer
 * @param buf  the buffer on which the CRC32 is calculated
 * @param len  the buffer len
 * @param init when calculating CRC32 on successive chunk to get back
 *             the CRC32 of the whole input content, contains the previous
 *             chunk CRC32, or 0xffffffff for the first one
 */

uint32_t crc32 (const unsigned char *buf, uint32_t len, uint32_t init);

// FIXME: add hmac


/***********************************************************
 * Firmware header manipulation functions
 **********************************************************/

#define FW_IV_LEN 16
#define FW_HMAC_LEN 32

typedef struct __packed {
	uint32_t magic;
	uint32_t type;
	uint32_t version;
	uint32_t len;
	uint32_t siglen;
	uint32_t chunksize;
	uint8_t iv[FW_IV_LEN];
	uint8_t hmac[FW_HMAC_LEN];
	/* The signature goes here ... with a siglen length */
} firmware_header_t;

/**
 * \brief parse the given buffer (starting with the firmware header)
 *
 * This function handle the header informations and store them in the
 * header pointer, taking endianess into account if needed.
 *
 * \param buffer the input buffer to parse
 * \param len    the output header max len
 * \param siglen the output signature max len
 * \param header the output header (to be fullfill)
 * \param sig    the output signature (to be fullfill)
 *
 */
int firmware_parse_header(__in  const uint8_t     *buffer,
                          __in  const uint32_t     len,
                          __in  const uint32_t     siglen,
                          __out firmware_header_t *header,
                          __out uint8_t           *sig);
/**
 *
 */
void firmware_print_header(const firmware_header_t * header);

#endif/*!LIBFW_H_*/
