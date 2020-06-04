/*
 * Copyright (c) 2016, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lprefix.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lstate.h"

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <lua_runtime_ta.h>

static int read_in_file(char* filename, unsigned char** out, long* outlen){

	FILE* f = fopen (filename, "rb");
	if (f){
		fseek(f, 0, SEEK_END);
		*outlen = ftell (f);
		fseek(f, 0, SEEK_SET);
		*out = malloc(*outlen);
		if (*out){
			fread (*out, 1, *outlen, f);
		}
		fclose (f);
	}

	return 0;
}

int invoke_script_number(unsigned char* script, size_t scriptlen, TEEC_Session *sess_ptr, int b_encrypted, int number){  
	
	uint32_t err_origin;
	TEEC_Operation op = {0};
	op.params[0].tmpref.buffer = script;
	op.params[0].tmpref.size = scriptlen;  
	op.params[1].value.a = number;
	op.params[2].value.a = b_encrypted;

	op.paramTypes = TEEC_PARAM_TYPES(
		TEEC_MEMREF_TEMP_INPUT,
		TEEC_VALUE_INOUT,
		TEEC_VALUE_INPUT,
		TEEC_NONE
	);


	TEEC_Result res = TEEC_InvokeCommand(sess_ptr, TA_RUN_LUA_SCRIPT_MATH, &op,
				 &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);

	printf("Lua script in TA returned: %d\n", op.params[1].value.b);

	return 0;

}

int invoke_saved_script_number(TEEC_Session *sess_ptr, char* script_name, int number){  
	
	uint32_t err_origin;
	TEEC_Operation op = {0};

	op.params[0].tmpref.buffer = script_name;
	op.params[0].tmpref.size = strlen(script_name);  
	op.params[1].value.a = number;

	op.paramTypes = TEEC_PARAM_TYPES(
		TEEC_MEMREF_TEMP_INPUT,
		TEEC_VALUE_INOUT,
		TEEC_NONE,
		TEEC_NONE
	);


	TEEC_Result res = TEEC_InvokeCommand(sess_ptr, TA_RUN_SAVED_LUA_SCRIPT_MATH, &op,
				 &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);

	printf("Lua script in TA returned: %d\n", op.params[1].value.b);

	return 0;

}

int save_script(unsigned char* script, size_t scriptlen, TEEC_Session *sess_ptr, int b_encrypted, char* script_name){

	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_VALUE_INPUT,
					 TEEC_NONE);

	op.params[0].tmpref.buffer = script_name;
	op.params[0].tmpref.size = strlen(script_name);

	op.params[1].tmpref.buffer = script;
	op.params[1].tmpref.size = scriptlen;

	op.params[2].value.a = b_encrypted;

	res = TEEC_InvokeCommand(sess_ptr,
				 TA_SAVE_LUA_SCRIPT,
				 &op, &origin);

	switch (res) {
	case TEEC_SUCCESS:
		break;
	default:
		printf("Command WRITE_RAW failed: 0x%x / %u\n", res, origin);
	}

	return res;

}


int trusted_pcall(char* script, TEEC_Session *sess_ptr, TEEC_Context *ctx_ptr){

	/* Prepares and sends an entire lua stack to the pa to be processed */

	// uint32_t err_origin;
	// TEEC_SharedMemory shm = {0};
	// TEEC_Operation op = {0};
	// const char *src = "HOST";
	// char *dst;

	
	// op.paramTypes = TEEC_PARAM_TYPES(
	// 	TEEC_MEMREF_TEMP_INOUT,
	// 	TEEC_VALUE_INPUT,
	// 	TEEC_VALUE_INPUT,
	// 	TEEC_NONE
	// );
	
	// /* create Lua state to be sent to the ta*/
	// lua_State *L = luaL_newstate();  
  	// if (L == NULL) {
    // 	printf("cannot create state: not enough memory");
	// }

	// int stateSize = L->l_G->totalbytes;

	// luaL_openlibs(L);
	
	// /* Load the lua script from the buffer and push it*/
	// luaL_loadbuffer(L, script, strlen(script), "lua_script");
	
	// /* Push the arguments on the stack*/
	// lua_pushnumber(L, 6);


 	// /* Set arguments for pcall */
	// op.params[1].value.a = 1; // Number of paramaters
	// op.params[1].value.b = 1; // Number of return values
	// op.params[2].value.b = 0; // Index of error function
	


	// /*Load Lua state into memory buffer*/
    // op.params[0].tmpref.buffer = L;
	// op.params[0].tmpref.size = stateSize;  

	

	// //lua_pcall((lua_State*)shm.buffer,1,1,0);
	// //lua_pcall(L,1,1,0);

	// /* Execute pcall on the stack in the secure environment */
	// TEEC_InvokeCommand(sess_ptr, TA_PCALL, &op, &err_origin);


	// /* Dealing with return values */
	// //dst = calloc(4, sizeof(char));
	// //memcpy(dst, shm.buffer, 4);
	// printf("Lua script in TA returned: %f\n", lua_tonumber(L, -1));

	
	// /* Cleanup */
	// TEEC_ReleaseSharedMemory(&shm);
	// free(shm.buffer);
	// //free(dst);
	// //lua_close(L); 

	return 0;
}


int main(void)
{

	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_UUID uuid = TA_LUA_RUNTIME_UUID;
	uint32_t err_origin;

	unsigned char* script = NULL;
	long scriptlen;

	unsigned char* encscript = NULL;
	long encscriptlen;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/*
	 * Open a session
	 */
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);

	
	read_in_file("script.lua", &script, &scriptlen);
	read_in_file("encrypted.luata", &encscript, &encscriptlen);

	/* Example function calls, mainly used for testing */

	//invoke_script_number(script, scriptlen,&sess, LUA_MODE_PLAINTEXT, 6);
	invoke_script_number(encscript,encscriptlen,&sess, LUA_MODE_ENCRYPTED, 6);

	//save_script(script, scriptlen, &sess, LUA_MODE_PLAINTEXT, "cubic");
	
	//save_script(encscript,encscriptlen, &sess, LUA_MODE_ENCRYPTED, "cubic2");

	//invoke_saved_script_number(&sess, "cubic2", 20);

	//trusted_pcall(script , &sess, &ctx);


	/* Cleanup session and context */
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	free(script);
	free(encscript);

	return 0;
}

