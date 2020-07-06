
#ifndef CRYPTOUTILS_H_INCLUDED
#define CRYPTOUTILS_H_INCLUDED

#include <inttypes.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <stdio.h>


/**
 * Function taken from https://github.com/linaro-swg/optee_examples/blob/master/hotp/ta/hotp_ta.c
 *  HMAC a block of memory to produce the authentication
 * 
 *  @param key       The secret key
 *  @param keylen    The length of the secret key (bytes)
 *  @param in        The data to HMAC
 *  @param inlen     The length of the data to HMAC (bytes)
 *  @param out       [out] Destination of the authentication tag
 *  @param outlen    [in/out] Max size and resulting size of authentication tag
 */
TEE_Result hmac_sha512(const uint8_t *key, const size_t keylen,
			    const uint8_t *in, const size_t inlen,
			    uint8_t *out, uint32_t *outlen);

/**
 *  Check the mac of the payload and decrypt it using keys generated with hkdf, generating a plaintext lua script
 *  @param buffer        The read file buffer: [salt (16 Bytes)][mac (64 Bytes)][nonce (8 Byte)][aes encrypted lua script]
 *  @param bufferlen     The length of the buffer
 *  @param out           [out] Destination of the plaintext lua script
 *  @param outlen        [in/out] Max size and resulting size of plaintext script
 */
TEE_Result verify_and_decrypt_script(uint8_t *payload, const size_t payloadlen, uint8_t *out, uint32_t *outlen);

#endif