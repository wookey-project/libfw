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
#include "fw_header.h"

void firmware_print_header(const firmware_header_t * header)
{
	if(header == NULL) {
		return;
	}
	printf("MAGIC = %x", header->magic);
	printf("\nTYPE  = %x", header->type);
	printf("\nVERSION = %x", header->version);
	printf("\nLEN = %x", header->len);
	printf("\nSIGLEN = %x", header->siglen);
	printf("\nCHUNKSIZE = %x", header->chunksize);	
	printf("\nIV = ");
	hexdump((unsigned char*)&(header->iv), 16);
	printf("\nHMAC = ");
	hexdump((unsigned char*)&(header->hmac), 16);

    return;
}

int firmware_parse_header(__in  const uint8_t     *buffer,
                          __in  const uint32_t     len,
                          __in  const uint32_t     siglen,
                          __out firmware_header_t *header,
                          __out uint8_t           *sig)
{
	/* Some sanity checks */
	if((buffer == NULL) || (header == NULL) || (sig == NULL)) {
		goto err;
	}
	if(len < sizeof(firmware_header_t)) {
		goto err;
	}
	/* Copy the header from the buffer */
	memcpy(header, buffer, sizeof(firmware_header_t));
	/* FIXME: define arch independent endianess management (to_device(xxx) instead of to_big/to_little */
	header->siglen    = to_big32(header->siglen);
	header->chunksize = to_big32(header->chunksize);
	header->len       = to_big32(header->len);
	header->magic     = to_big32(header->magic);
	/* Get the signature length */
	if(header->siglen > siglen) {
		/* Not enough room to store the signature */
		goto err;
	}
	if(len < sizeof(firmware_header_t)+header->siglen) {
		/* The provided buffer is too small! */
		goto err;
	} 
	/* Copy the signature */
	memcpy(sig, buffer+sizeof(firmware_header_t), header->siglen);

	return 0;
err:
	return -1;
}
