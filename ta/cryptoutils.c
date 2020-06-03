#include <inttypes.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cryptoutils.h"

/* For testing purposes only, non critical key*/
#define SYM_KEY {\
	0x43, 0x2a, 0x46, 0x2d, 0x4a, 0x61, 0x4e, 0x64,\
	0x52, 0x67, 0x55, 0x6a, 0x58, 0x6e, 0x32, 0x72,\
	0x35, 0x75, 0x38, 0x78, 0x2f, 0x41, 0x3f, 0x44,\
	0x28, 0x47, 0x2b, 0x4b, 0x62, 0x50, 0x65, 0x53}


#define KEY_LEN 256
#define AES_BLOCK_SIZE		16


/*
 * Function taken from https://github.com/linaro-swg/optee_examples/blob/master/hotp/ta/hotp_ta.c
 *  HMAC a block of memory to produce the authentication
 *  @param key       The secret key
 *  @param keylen    The length of the secret key (bytes)
 *  @param in        The data to HMAC
 *  @param inlen     The length of the data to HMAC (bytes)
 *  @param out       [out] Destination of the authentication tag
 *  @param outlen    [in/out] Max size and resulting size of authentication tag
 */
TEE_Result hmac_sha512(const uint8_t *key, const size_t keylen,
			    const uint8_t *in, const size_t inlen,
			    uint8_t *out, uint32_t *outlen)
{
	TEE_Attribute attr = { 0 };
	TEE_ObjectHandle key_handle = TEE_HANDLE_NULL;
	TEE_OperationHandle op_handle = TEE_HANDLE_NULL;
	TEE_Result res = TEE_SUCCESS;

	if (!in || !out || !outlen)
		return TEE_ERROR_BAD_PARAMETERS;

	/*
	 * 1. Allocate cryptographic (operation) handle for the HMAC operation.
	 *    Note that the expected size here is in bits (and therefore times
	 *    8)!
	 */
	res = TEE_AllocateOperation(&op_handle, TEE_ALG_HMAC_SHA512, TEE_MODE_MAC,
				    keylen * 8);
	if (res != TEE_SUCCESS) {
		EMSG("0x%08x", res);
		goto exit;
	}

	/*
	 * 2. Allocate a container (key handle) for the HMAC attributes. Note
	 *    that the expected size here is in bits (and therefore times 8)!
	 */
	res = TEE_AllocateTransientObject(TEE_TYPE_HMAC_SHA512, keylen * 8,
					  &key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("0x%08x", res);
		goto exit;
	}

	/*
	 * 3. Initialize the attributes, i.e., point to the actual HMAC key.
	 *    Here, the expected size is in bytes and not bits as above!
	 */
	TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, key, keylen);

	/* 4. Populate/assign the attributes with the key object */
	res = TEE_PopulateTransientObject(key_handle, &attr, 1);
	if (res != TEE_SUCCESS) {
		EMSG("0x%08x", res);
		goto exit;
	}

	/* 5. Associate the key (object) with the operation */
	res = TEE_SetOperationKey(op_handle, key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("0x%08x", res);
		goto exit;
	}

	/* 6. Do the HMAC operations */
	TEE_MACInit(op_handle, NULL, 0);
	TEE_MACUpdate(op_handle, in, inlen);
	res = TEE_MACComputeFinal(op_handle, NULL, 0, out, outlen);
exit:
	if (op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(op_handle);

	/* It is OK to call this when key_handle is TEE_HANDLE_NULL */
	TEE_FreeTransientObject(key_handle);

	return res;
}



// TODO tidy up this functions and take better care of error handeling

/*
 *  Check the mac of the payload and decrypt it using keys generated with hkdf, generating a plaintext lua script
 *  @param buffer        The read file buffer: [salt (16 Bytes)][mac (64 Bytes)][nonce (8 Byte)][aes encrypted lua script]
 *  @param bufferlen     The length of the buffer
 *  @param out       [out] Destination of the plaintext lua script
 *  @param outlen    [in/out] Max size and resulting size of plaintext script
 */
TEE_Result verify_and_decrypt_script(uint8_t *buffer, const size_t bufferlen, uint8_t *out, uint32_t *outlen)
{

	TEE_Result res;

	TEE_OperationHandle hkdf_op_handle = TEE_HANDLE_NULL;
	TEE_ObjectHandle master_handle = TEE_HANDLE_NULL;
	TEE_ObjectHandle hkdf_result_handle = TEE_HANDLE_NULL;
	TEE_Attribute master_attr;
	TEE_Attribute hkdf_attrs[2];

	uint8_t okm_buffer[KEY_LEN/4] = {0};
	size_t okm_len = KEY_LEN/4;

	

	// Decryption
	TEE_OperationHandle aes_op_handle = TEE_HANDLE_NULL;
	TEE_ObjectHandle aes_key_handle= TEE_HANDLE_NULL;
	TEE_Attribute aes_attr;

	unsigned char master_key[KEY_LEN / 8] = SYM_KEY;

	size_t salt_sz = 16; 
	unsigned char salt[16];
	size_t mac_sz = 64; 
	unsigned char mac[64];
	
	uint8_t mac_cmp_buffer[64] = {0};
	size_t mac_cmp_len = mac_sz;

	// iv = nonce + counter https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation#/media/File:CTR_encryption_2.svg
	size_t nonce_sz = 8;
	size_t iv_sz = 16;
	unsigned char iv[16];
	

	/*extract salt (HKDF) and mac (HMAC) from the buffer*/
	TEE_MemMove(salt,buffer, salt_sz);
	TEE_MemMove(mac,buffer+salt_sz, mac_sz);

	/*fill the first nonce_sz byte of the iv with the attached value*/
	TEE_MemMove(iv,buffer+salt_sz+mac_sz, nonce_sz);


	/* HKDF to derive the two needed keys from the master key*/

	res = TEE_AllocateOperation(&hkdf_op_handle,
				    TEE_ALG_HKDF_SHA512_DERIVE_KEY,
				    TEE_MODE_DERIVE,
				    KEY_LEN);

	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate operation");
		hkdf_op_handle = TEE_HANDLE_NULL;
		return 1;
	}

	TEE_AllocateTransientObject(TEE_TYPE_HKDF_IKM,
					  KEY_LEN,
	 				  &master_handle);


	TEE_InitRefAttribute(&master_attr, TEE_ATTR_HKDF_IKM, master_key, KEY_LEN/8);
	
	TEE_ResetTransientObject(master_handle);
	res = TEE_PopulateTransientObject(master_handle, &master_attr, 1);
	if (res != TEE_SUCCESS) {
	 	EMSG("TEE_PopulateTransientObject failed, %x", res);
	 	return res;
	}
	
	
	res = TEE_SetOperationKey(hkdf_op_handle, master_handle);
	if (res != TEE_SUCCESS) {
	 	EMSG("TEE_SetOperationKey failed %x", res);
	 	return res;
	}

	TEE_InitValueAttribute(&hkdf_attrs[0], TEE_ATTR_HKDF_OKM_LENGTH, KEY_LEN/4, 0);
	TEE_InitRefAttribute(&hkdf_attrs[1], TEE_ATTR_HKDF_SALT, salt, 16);
	
	res = TEE_AllocateTransientObject(TEE_TYPE_GENERIC_SECRET,
					  KEY_LEN*2,
	 				  &hkdf_result_handle);

	
	TEE_DeriveKey(hkdf_op_handle, hkdf_attrs, 2, hkdf_result_handle);
	
	TEE_GetObjectBufferAttribute(hkdf_result_handle, TEE_ATTR_SECRET_VALUE, okm_buffer, &okm_len);


	/* Generate HMAC over nonce + payload*/
	
	hmac_sha512(okm_buffer+32, 32,
			    buffer + salt_sz + mac_sz, bufferlen - (salt_sz + mac_sz),
			    mac_cmp_buffer, &mac_cmp_len);


	/* Check if the generated hash matches the attached MAC */
	res = TEE_MemCompare(mac, mac_cmp_buffer, mac_cmp_len);
	if (res != TEE_SUCCESS) {
		EMSG("MAC did not match the data");
		goto err;
	}

	//for(int i = 0; i < mac_cmp_len; i++){
	//	printf("MAC (%02d): %02X\n", i, ( (unsigned char*) &(mac_cmp_buffer)) [i]);
	//}


	/* Decryption */
	
	res = TEE_AllocateOperation(&aes_op_handle,
				    TEE_ALG_AES_CTR,
				    TEE_MODE_DECRYPT,
				    KEY_LEN);

	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate operation");
		aes_op_handle = TEE_HANDLE_NULL;
		goto err;
	}


	res = TEE_AllocateTransientObject(TEE_TYPE_AES,
					  KEY_LEN,
					  &aes_key_handle);

	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate transient object");
		aes_key_handle = TEE_HANDLE_NULL;
		goto err;
	}	

	TEE_InitRefAttribute(&aes_attr, TEE_ATTR_SECRET_VALUE, okm_buffer, KEY_LEN/8);
	TEE_ResetTransientObject(aes_key_handle);
	res = TEE_PopulateTransientObject(aes_key_handle, &aes_attr, 1);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_PopulateTransientObject failed, %x", res);
		return res;
	}

	
	res = TEE_SetOperationKey(aes_op_handle, aes_key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_SetOperationKey failed %x", res);
		goto err;
	}


	TEE_CipherInit(aes_op_handle, iv, iv_sz);
	
	res = TEE_CipherUpdate(aes_op_handle, buffer+salt_sz+mac_sz+nonce_sz, bufferlen-(salt_sz+mac_sz+nonce_sz), out, outlen);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_CipherUpdate failed 0x%08x", res);
		return res;
	}
	
	return TEE_SUCCESS;
	

err:
	if (aes_op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(aes_op_handle);
	aes_op_handle = TEE_HANDLE_NULL;

	if (aes_key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(aes_key_handle);
	aes_key_handle = TEE_HANDLE_NULL;

	return res;

}