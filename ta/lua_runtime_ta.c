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


#include <inttypes.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <lua_runtime_ta.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"

#include "lprefix.h"
#include "lauxlib.h"
#include "lualib.h"

#include "cryptoutils.h"
#include "lua_arguments.h"




void MSG_LUA_ERROR(lua_State *L, char *msg){
	MSG("\nFATAL ERROR:\n  %s: %s\n\n",
		msg, lua_tostring(L, -1));
}

int internal_TA_call(lua_State *L) {

	void* lua_arg;
	int lua_arg_type;
	void* lua_ret; 
	int lua_ret_type;

	char* script_name = luaL_checkstring(L, 1); 
	
	args_from_stack(L, 2, &lua_arg, &lua_arg_type);
	
	run_saved_lua_script(script_name, strlen(script_name),lua_arg, lua_arg_type, &lua_ret, &lua_ret_type);
	
	stack_from_args(L, lua_ret, lua_ret_type);	

	return 1;  /* number of results */
}

void call_lua(char* script, size_t script_len, void* input, int input_type, void** output, int* output_type){

	lua_State *L = luaL_newstate();  /* create Lua state */
  	if (L == NULL) {
    	MSG("cannot create state: not enough memory");
	}

	luaL_openlibs(L);

	/* Register the function for calling interal Lua scripts with the state */
	lua_pushcfunction(L, internal_TA_call);
    lua_setglobal(L, "internal_TA_call");
	
	/* Load the lua script from the buffer */
	luaL_loadbuffer(L, script, script_len, "lua_script"); 
	
	/* Push argument on the stack */
	stack_from_args(L, input, input_type);

    if (lua_pcall(L, 1, 1, 0)){            
		MSG_LUA_ERROR(L, "lua_pcall() failed"); 
		printf("%s", script);
	}
		
	/* Return value of operation */
	args_from_stack(L, -1 ,output, output_type);
	
    lua_close(L); 
}

/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void){}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA. In this function you will normally do the global initialization for the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param __maybe_unused params[4],
		void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;
	


	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx;
}


TEE_Result run_lua_script(uint32_t param_types,
	TEE_Param params[4])
{	

	TEE_Result res;
	char* script;
	uint32_t script_len;

	/* local buffer to temporarily copy the shared buffer from the client side. Makes sure shared memory is only read once */
	size_t buffer_size;

	char* local_buffer;
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, 
						   TEE_PARAM_TYPE_VALUE_INOUT,
						   TEE_PARAM_TYPE_VALUE_INPUT, 
						   TEE_PARAM_TYPE_MEMREF_INOUT 
						   );

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	buffer_size = params[0].memref.size;
	local_buffer = TEE_Malloc(buffer_size, 0);
	TEE_MemMove(local_buffer, params[0].memref.buffer, buffer_size); 

	script = local_buffer;
	script_len = buffer_size;

	/* If the buffer is not a plaintext lua script, verify and decypher the buffer first*/
	if(params[2].value.a){
		script = TEE_Malloc(script_len, 0);
		verify_and_decrypt_script(local_buffer, buffer_size , script, &script_len);
	}

	void* lua_arg;
	void* lua_ret;

	args_from_params_ta(&lua_arg, params);

	call_lua(script, script_len, lua_arg, params[1].value.a, &lua_ret, &params[1].value.a);

	params_from_args_ta(lua_ret, params[1].value.a, params);
	
	if(params[2].value.a){
		TEE_Free(script);	
	}
	
	return TEE_SUCCESS;
}


TEE_Result save_lua_script(uint32_t param_types,
	TEE_Param params[4])
{	

	uint32_t exp_param_types = TEE_PARAM_TYPES(
		TEE_PARAM_TYPE_MEMREF_INPUT,
		TEE_PARAM_TYPE_MEMREF_INPUT,
		TEE_PARAM_TYPE_VALUE_INPUT,
		TEE_PARAM_TYPE_NONE
		);

	TEE_ObjectHandle object;
	TEE_Result res;
	char *script_name;
	size_t script_name_sz;
	char *data;
	size_t data_sz;
	uint32_t obj_data_flag;

	/* local buffer to temporarily copy the shared buffer from the client side. Makes sure shared memory is only read once */
	size_t buffer_size;
	char* local_buffer;
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Get script name from parameters */
	script_name_sz = params[0].memref.size;
	script_name = TEE_Malloc(script_name_sz, 0);
	if (!script_name)
		return TEE_ERROR_OUT_OF_MEMORY;
	TEE_MemMove(script_name, params[0].memref.buffer, script_name_sz);

	buffer_size = params[1].memref.size;
	local_buffer = TEE_Malloc(buffer_size, 0);
	TEE_MemMove(local_buffer, params[1].memref.buffer, buffer_size); 

	data = local_buffer;
	data_sz = buffer_size;

	/* if the buffer is not a plaintext lua script, verify and decypher the buffer first*/
	if(params[2].value.a){
		data = TEE_Malloc(data_sz, 0);
		verify_and_decrypt_script(local_buffer, buffer_size, data, &data_sz);
	}


	/*
	 * Create object in secure storage and fill with data
	 */
	obj_data_flag = TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_ACCESS_WRITE |
					TEE_DATA_FLAG_ACCESS_WRITE_META |
					TEE_DATA_FLAG_OVERWRITE;		
			

	res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
					script_name, script_name_sz,
					obj_data_flag,
					TEE_HANDLE_NULL,
					NULL, 0,		
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_CreatePersistentObject failed 0x%08x", res);
		TEE_Free(script_name);
		return res;
	}

	/* Save the object to secure storage */
	res = TEE_WriteObjectData(object, data, data_sz);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_WriteObjectData failed 0x%08x", res);
		TEE_CloseAndDeletePersistentObject1(object);
	} else {
		TEE_CloseObject(object);
	}
	TEE_Free(script_name);
	

	return TEE_SUCCESS;
}


TEE_Result run_saved_lua_script_entry(uint32_t param_types,
	TEE_Param params[4])
{	

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, 
						   TEE_PARAM_TYPE_VALUE_INOUT, 
						   TEE_PARAM_TYPE_VALUE_INPUT, 
						   TEE_PARAM_TYPE_MEMREF_INOUT
						   );

	TEE_Result res;
	char *script_name;
	size_t script_name_sz;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Get script name from parameters */
	script_name_sz = params[0].memref.size;
	script_name = TEE_Malloc(script_name_sz, 0);
	if (!script_name)
		return TEE_ERROR_OUT_OF_MEMORY;
	TEE_MemMove(script_name, params[0].memref.buffer, script_name_sz);

	void* lua_arg;
	void* lua_ret;

	args_from_params_ta(&lua_arg, params);

	run_saved_lua_script(script_name, script_name_sz, lua_arg, params[1].value.a, &lua_ret, &params[1].value.a);

	params_from_args_ta(lua_ret, params[1].value.a, params);

	TEE_Free(script_name);
	return TEE_SUCCESS;

exit:
	TEE_Free(script_name);
	return res;
}

TEE_Result run_saved_lua_script(char* script_name, size_t script_name_sz, void* input, int input_type, void** output, int* output_type)
{	

	TEE_ObjectHandle object;
	TEE_ObjectInfo object_info;
	TEE_Result res;
	uint32_t read_bytes;
	char *data;
	size_t data_sz;

	/*
	 * Check the object exist and can be dumped into output buffer
	 * then dump it.
	 */
	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					script_name, script_name_sz,
					TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_SHARE_READ,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open persistent object, res=0x%08x", res);
		return res;
	}

	res = TEE_GetObjectInfo1(object, &object_info);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to create persistent object, res=0x%08x", res);
		goto exit;
	}

	data = (char*) TEE_Malloc(object_info.dataSize, TEE_MALLOC_FILL_ZERO);


	res = TEE_ReadObjectData(object, data, object_info.dataSize,
				 &read_bytes);
	if (res != TEE_SUCCESS || read_bytes != object_info.dataSize) {
		EMSG("TEE_ReadObjectData failed 0x%08x, read %" PRIu32 " over %u",
				res, read_bytes, object_info.dataSize);
		goto exit;
	}
	
	call_lua(data, read_bytes, input, input_type, output, output_type);

	return TEE_SUCCESS;

exit:
	TEE_CloseObject(object);
	return res;
}

/* TODO redo for benchmarking and comparison */
TEE_Result run_ta(uint32_t param_types,
	TEE_Param params[4])
{	
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE, 
						   TEE_PARAM_TYPE_VALUE_INOUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE 
						   );

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
	
	int a = params[1].value.a;
	for(int i = 0; i<10000000; i++){
		a = a + 1;
		asm("");
	}
	params[1].value.b = a;
	return TEE_SUCCESS;

}



/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx;
	switch (cmd_id) {
	case TA_RUN_LUA_SCRIPT:
		return run_lua_script(param_types, params);
	case TA_RUN_SAVED_LUA_SCRIPT:
		return run_saved_lua_script_entry(param_types, params);
	case TA_SAVE_LUA_SCRIPT:
		return save_lua_script(param_types, params);
	case TA:
		return run_ta(param_types, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
