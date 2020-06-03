
#ifndef CRYPTOUTILS_H_INCLUDED
#define CRYPTOUTILS_H_INCLUDED

#include <inttypes.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <stdio.h>


TEE_Result hmac_sha512(const uint8_t *key, const size_t keylen,
			    const uint8_t *in, const size_t inlen,
			    uint8_t *out, uint32_t *outlen);

TEE_Result verify_and_decrypt_script(uint8_t *payload, const size_t payloadlen, uint8_t *out, uint32_t *outlen);

#endif