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
#include "libc/stdio.h"
#include "libc/nostd.h"
#include "libc/string.h"
#include "libc/nostd.h"
#include "libc/arpa/inet.h"

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
	if((buffer == NULL) || (header == NULL)) {
		goto err;
	}
	if(len < sizeof(firmware_header_t)) {
		goto err;
	}


    /* managing header structure */
	/* Copy the header from the buffer */
	memcpy(header, buffer, sizeof(firmware_header_t));
	/* FIXME: define arch independent endianess management (to_device(xxx) instead of to_big/to_little */
	header->siglen    = htonl(header->siglen);
	header->chunksize = htonl(header->chunksize);
	header->len       = htonl(header->len);
	header->magic     = htonl(header->magic);
	header->type      = htonl(header->type);
	header->version   = htonl(header->version);
    if (sig != NULL) {
        /* full header size */
        if(len < (sizeof(firmware_header_t)+header->siglen)) {
            /* The provided buffer is too small! */
            goto err;
        }
    } else {
        /* truncated header size, without signature */
        if(len < sizeof(firmware_header_t)) {
            /* The provided buffer is too small! */
            goto err;
        }
    }

    /* managing signature */
    if (sig != NULL) {
        if(header->siglen > siglen) {
            /* Not enough room to store the signature */
            goto err;
        }
        /* Copy the signature */
        memcpy(sig, buffer+sizeof(firmware_header_t), header->siglen);
    }

	return 0;
err:
	return -1;
}

int firmware_header_to_raw(__in const firmware_header_t *header,
			   __out  uint8_t     *buffer,
                           __out  const uint32_t     len)
{
	/* Some sanity checks */
	if((buffer == NULL) || (header == NULL)) {
		goto err;
	}
	if(len < sizeof(firmware_header_t)) {
		goto err;
	}

	/* Now copy the fields with reversed endianness */
	memcpy(buffer, header, sizeof(firmware_header_t));
	*((uint32_t*)buffer) = htonl(*((uint32_t*)buffer));
	buffer += 4;
	*((uint32_t*)buffer) = htonl(*((uint32_t*)buffer));
	buffer += 4;
	*((uint32_t*)buffer) = htonl(*((uint32_t*)buffer));
	buffer += 4;
	*((uint32_t*)buffer) = htonl(*((uint32_t*)buffer));
	buffer += 4;
	*((uint32_t*)buffer) = htonl(*((uint32_t*)buffer));
	buffer += 4;
	*((uint32_t*)buffer) = htonl(*((uint32_t*)buffer));

	return 0;
err:
	return -1;
}

bool firmware_is_partition_flip(__in const firmware_header_t *header)
{
	if(header == NULL){
		goto err;
	}
	if(header->type == PART_FLIP){
		return true;
	}
	else{
		return false;
	}

err:
	return false;
}

bool firmware_is_partition_flop(__in const firmware_header_t *header)
{
	if(header == NULL){
		goto err;
	}
	if(header->type == PART_FLOP){
		return true;
	}
	else{
		return false;
	}

err:
	return false;
}

