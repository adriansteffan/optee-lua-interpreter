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


void MSG_LUA_ERROR(lua_State *L, char *msg){
	MSG("\nFATAL ERROR:\n  %s: %s\n\n",
		msg, lua_tostring(L, -1));
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

static TEE_Result run_lua_script_math(uint32_t param_types,
	TEE_Param params[4])
{	
	/* Temporary example: Calculata a R -> R function defined in the lua script */

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, /* Reference to the buffer containing the lua script */
						   TEE_PARAM_TYPE_VALUE_INPUT, /* Input parameter, a singular number for now */
						   TEE_PARAM_TYPE_VALUE_OUTPUT, /* Output paramater, a singular number for now */ 
						   TEE_PARAM_TYPE_NONE /* unused slot */
						   );

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
	
	lua_State *L = luaL_newstate();  /* create Lua state */
  	if (L == NULL) {
    	MSG("cannot create state: not enough memory");
	}

	luaL_openlibs(L);
	
	/* Load the lua script from the buffer */
	luaL_loadbuffer(L, params[0].memref.buffer, params[0].memref.size, "lua_script");
	
	lua_pushnumber(L, params[1].value.a);                      
    if (lua_pcall(L, 1, 1, 0))                  
		MSG_LUA_ERROR(L, "lua_pcall() failed"); 
 
	
	params[2].value.a = lua_tonumber(L, -1);
	

    lua_close(L); 


	return TEE_SUCCESS;
}


static TEE_Result save_lua_script(uint32_t param_types,
	TEE_Param params[4])
{	
	/* Saves a lua function to the secure staorage to be invoke by later calls to the ta */

	uint32_t exp_param_types = TEE_PARAM_TYPES(
		TEE_PARAM_TYPE_MEMREF_INPUT, /* The name of the script under which the function will be accessible later */
		TEE_PARAM_TYPE_MEMREF_INPUT, /* The contents of the lua script */
		TEE_PARAM_TYPE_NONE,
		TEE_PARAM_TYPE_NONE
		);

	TEE_ObjectHandle object;
	TEE_Result res;
	char *script_name;
	size_t script_name_sz;
	char *data;
	size_t data_sz;
	uint32_t obj_data_flag;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	
	script_name_sz = params[0].memref.size;
	script_name = TEE_Malloc(script_name_sz, 0);
	if (!script_name)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(script_name, params[0].memref.buffer, script_name_sz);

	data = (char *)params[1].memref.buffer;
	data_sz = params[1].memref.size;

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


static TEE_Result run_saved_lua_script_math(uint32_t param_types,
	TEE_Param params[4])
{	
	/* Temporary example: Calculata a R -> R function defined in the saved lua script */

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, /* name of the file in the trusted storage containing the lua script */
						   TEE_PARAM_TYPE_VALUE_INPUT, /* Input parameter, a singular number for now */
						   TEE_PARAM_TYPE_VALUE_OUTPUT, /* Output paramater, a singular number for now */ 
						   TEE_PARAM_TYPE_NONE /* unused slot */
						   );

	TEE_ObjectHandle object;
	TEE_ObjectInfo object_info;
	TEE_Result res;
	uint32_t read_bytes;
	char *script_name;
	size_t script_name_sz;
	char *data;
	size_t data_sz;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;



	script_name_sz = params[0].memref.size;
	script_name = TEE_Malloc(script_name_sz, 0);
	if (!script_name)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(script_name, params[0].memref.buffer, script_name_sz);


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
		TEE_Free(script_name);
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


	
	lua_State *L = luaL_newstate();  /* create Lua state */
  	if (L == NULL) {
    	MSG("cannot create state: not enough memory");
	}

	luaL_openlibs(L);

	/* Load the lua script from the buffer */
	luaL_loadbuffer(L, data , read_bytes, "lua_script");
	
	lua_pushnumber(L, params[1].value.a);                      
    if (lua_pcall(L, 1, 1, 0))                  
		MSG_LUA_ERROR(L, "lua_pcall() failed"); 
 
	
	params[2].value.a = lua_tonumber(L, -1);
	

    lua_close(L); 


	return TEE_SUCCESS;

exit:
	TEE_CloseObject(object);
	TEE_Free(script_name);
	return res;
}



static TEE_Result trusted_pcall(uint32_t param_types,
	TEE_Param params[4])
{	
	
	//params[0].memref.buffer;

	//if (lua_pcall(L, params[1].value.a, params[1].value.b, params[2].value.a))                  
	//	MSG_LUA_ERROR(L, "lua_pcall() failed"); 

	//dst = TEE_Malloc(4, TEE_MALLOC_FILL_ZERO);
	//TEE_MemMove(dst, tmp, 4);
	//TEE_MemMove(params[0].memref.buffer, dst, 4);


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
	case TA_PCALL:
		return trusted_pcall(param_types, params);
	case TA_RUN_LUA_SCRIPT_MATH:
		return run_lua_script_math(param_types, params);
	case TA_RUN_SAVED_LUA_SCRIPT_MATH:
		return run_saved_lua_script_math(param_types, params);
	case TA_SAVE_LUA_SCRIPT:
		return save_lua_script(param_types, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
