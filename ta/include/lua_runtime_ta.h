/*
 * Copyright (c) 2016-2017, Linaro Limited
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

#include "lua.h"

#include "lprefix.h"
#include "lauxlib.h"
#include "lualib.h"

#ifndef TA_LUA_RUNTIME_H
#define TA_LUA_RUNTIME_H


/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define TA_LUA_RUNTIME_UUID \
	{ 0xdebd5a03, 0xe1c1, 0x4e16, \
		{ 0x89, 0xa9, 0xc2, 0x94, 0xe3, 0xd7, 0x8c, 0xd5} }

#define BYTE_BUFFER_SIZE 8192


/* The function IDs implemented in this TA */

/*
 * TA_RUN_LUA_SCRIPT - Runs the input lua script inside the TA and fills the params with the output value
 * param[0] (memref) input buffer containing the encrypted (or plaintext) lua script
 * param[1] (value)  related to lua arguments, see lua_arguments.h for further details 
 * param[2] (value)  a: A flag to indicate if the input data is plaintext or encrypted+signed 
 * 					 b: unused
 * param[3] (memref) related to lua arguments, see lua_arguments.h for further details 
 */
#define TA_RUN_LUA_SCRIPT		1


/*
 * TA_RUN_SAVED_LUA_SCRIPT - Runs a lua script already present inside the TA and fills the params with the output value
 * param[0] (memref) input buffer containing the name of the lua script
 * param[1] (value)  related to lua arguments, see lua_arguments.h for further details 
 * param[2] unused
 * param[3] (memref) related to lua arguments, see lua_arguments.h for further details 
 */
#define TA_RUN_SAVED_LUA_SCRIPT		2

/*
 * TA_SAVE_LUA_SCRIPT - Saves a lua script in the secure storage of the TA for later calling
 * param[0] (memref) input buffer containing the name of the lua script
 * param[1] (memref) input buffer containing the encrypted (or plaintext) lua script
 * param[2] (value)  a: A flag to indicate if the input data is plaintext or encrypted+signed 
 * 					 b: unused
 * param[3] unused
 */
#define TA_SAVE_LUA_SCRIPT		3

/* Currently not working*/
#define TA	4

/* flag values to indicate wether a passed lua script needs to be decrypted before running */
#define LUA_MODE_PLAINTEXT	0
#define LUA_MODE_ENCRYPTED	1


#ifdef TRUSTED_APP_BUILD

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>


/* Entry function for TA_SAVE_LUA_SCRIPT*/
TEE_Result save_lua_script(uint32_t param_types, TEE_Param params[4]);

/* Entry function for TA_RUN_LUA_SCRIPT*/
TEE_Result run_lua_script(uint32_t param_types, TEE_Param params[4]);

/* Entry function for TA_RUN_SAVED_LUA_SCRIPT*/
TEE_Result run_saved_lua_script_entry(uint32_t param_types, TEE_Param params[4]);


/**
 * Calls another Lua script from inside a Lua script running in the TA. Only called by Lua scripts.
 *
 * @param L   [in/out] The Lua stack passed from the Lua script
 */
int internal_TA_call(lua_State *L); 


/**
 * Runs a Lua script with the given argument and gets the return value from the stack.
 *
 * @param script        [in] The Lua script to be run
 * @param script_len    [in] The length of the Lua script 
 * @param input         [in] A pointer to the input argument
 * @param input_type    [in] An integer flag indicating the type of the input argument  
 * @param output  		[out] A pointer to the pointer which will point to the return value 
 * @param output_type  	[out] An integer flag indicating the type of the return value
 */
void call_lua(char* script, size_t script_len, void* input, int input_type, void** output, int* output_type);


/**
 * Runs a Lua script already present in the interal TA storage with the given argument and gets the return value from the stack.
 *
 * @param script_name    [in] The name of the Lua script to be run
 * @param script_name_sz [in] The length of the name of the Lua script 
 * @param input          [in] A pointer to the input argument
 * @param input_type     [in] An integer flag indicating the type of the input argument  
 * @param output  		 [out] A pointer to the pointer which will point to the return value 
 * @param output_type  	 [out] An integer flag indicating the type of the return value 
 */
TEE_Result run_saved_lua_script(char* script_name, size_t script_name_sz, void* input, int input_type, void** output, int* output_type);
#endif

#endif /*TA_LUA_RUNTIME_H*/




