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
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <dirent.h>


#include "lua.h"
#include "lprefix.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lstate.h"

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <lua_runtime_ta.h>

#define CALL_MODE_PASS 	0
#define CALL_MODE_SAVED	 1

TEEC_Session sess;

/* flag to indicate wether the encrypted lua scripts should be used */
int encrypted_mode = LUA_MODE_ENCRYPTED;

/* flag to indicate wether called lua ta scripts should be passed in for each or loaded from the secure storage */
int call_mode = CALL_MODE_PASS;

char* app_name;


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

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}


/**
 * Saves a Lua script to the TA storage. It can later be called without needing to resend the script from the rich OS side.
 *
 * @param script         [in] The Lua script to be saved
 * @param scriptlen      [in] The length of the Lua script
 * @param b_encrypted    [in] An integer flag indicating wether the lua script is encrypted or plaintext.
 * @param script_name    [in] The name that the Lua script will be invoked with once saved
 */
int save_script(unsigned char* script, size_t scriptlen, int b_encrypted, char* script_name){

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

	res = TEEC_InvokeCommand(&sess,
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

/**
 * Runs a Lua script in the TA interpreter with the given argument and gets the return value from the returning params.
 *
 * @param script         [in] The Lua script OR the name of the Lua script to be run
 * @param scriptlen      [in] The length of the Lua script OR the length of the name of the Lua script
 * @param b_script_saved [in] An integer flag indicating wether to send in the Lua script or call one already saved in the secure TA storage.
 * 							  script and scriptlen are interpreted accordingly.
 * @param b_encrypted    [in] An integer flag indicating wether the lua script is encrypted or plaintext. (unused if b_script_saved == TRUE)
 * @param input          [in] A pointer to the input argument
 * @param input_type     [in] An integer flag indicating the type of the input argument  
 * @param output  		 [out] A pointer which will point to the return value 
 * @param output_type  	 [out] An integer flag indicating the type of the return value
 */
int invoke_script(unsigned char* script, size_t scriptlen, int b_script_saved, int b_encrypted, void* input, int input_type, void* output, int *output_type){  
	
	
	uint32_t err_origin;
	TEEC_Operation op = {0};
	int ta_command;

	op.paramTypes = TEEC_PARAM_TYPES(
		TEEC_MEMREF_TEMP_INPUT,
		TEEC_VALUE_INOUT,
		TEEC_VALUE_INPUT,
		TEEC_MEMREF_TEMP_INOUT /* memory buffer used for string values and lua code used as an argument */
	);

	
	ta_command = b_script_saved ? TA_RUN_SAVED_LUA_SCRIPT : TA_RUN_LUA_SCRIPT;
	

	op.params[0].tmpref.buffer = script;
	op.params[0].tmpref.size = scriptlen; 

	op.params[1].value.a = input_type; // will get replaced by output type
	
	op.params[2].value.a = b_encrypted;

	op.params[3].tmpref.buffer = malloc(BYTE_BUFFER_SIZE);
	op.params[3].tmpref.size = BYTE_BUFFER_SIZE;

	params_from_args_rich(input, input_type, op.params);

	struct timeval start, end;

    gettimeofday(&start, NULL);


	TEEC_Result res = TEEC_InvokeCommand(&sess, ta_command, &op,
			 &err_origin);
    gettimeofday(&end, NULL);


    double time_taken = end.tv_sec * 1e3 + end.tv_usec / 1e3 -
                        start.tv_sec* 1e3 - start.tv_usec / 1e3; // in milseconds


    printf("time program took %f milliseconds to execute\n", time_taken);
		
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);

	
	
	*output_type = op.params[1].value.a;
	
	args_from_params_rich(output, op.params);
	
	return 0;

}


/* for comparison purposes, currently broken, fix when benchmarking */
int invoke_ta_number(TEEC_Session *sess_ptr, int number, int* output){  
	
	uint32_t err_origin;
	TEEC_Operation op = {0};

	op.params[1].value.a = number;

	op.paramTypes = TEEC_PARAM_TYPES(
		TEEC_NONE,
		TEEC_VALUE_INOUT,
		TEEC_NONE,
		TEEC_NONE
	);

	struct timeval start, end;

    gettimeofday(&start, NULL);

	TEEC_Result res = TEEC_InvokeCommand(sess_ptr, TA, &op,
				 &err_origin);


    gettimeofday(&end, NULL);

    double time_taken = end.tv_sec * 1e3 + end.tv_usec / 1e3 -
                        start.tv_sec* 1e3 - start.tv_usec / 1e3; // in milseconds

    printf("time program took %f milliseconds to execute\n", time_taken);

	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);

	*output = op.params[1].value.b;

	return 0;

}

/**
 * Invokes a Lua TA from inside a Lua script on the rich OS side. Only called by Lua scripts.
 *
 * @param L   [in/out] The Lua stack passed from the Lua script
 */
static int TA_call(lua_State *L) {
	 
	void* lua_arg;
	int lua_arg_type;
	void* lua_ret = malloc(sizeof(void *)); 
	int lua_ret_type;

	char* script_name = luaL_checkstring(L, 1);

	args_from_stack(L, 2,&lua_arg, &lua_arg_type);

	if(call_mode == CALL_MODE_SAVED){
		invoke_script(script_name, strlen(script_name), CALL_MODE_SAVED, 0, lua_arg, lua_arg_type, lua_ret, &lua_ret_type);
	}else{

		/* For a performance increase, send in Lua TA scripts that are called instead of invoking scripts saved in the internal TA storage */
		/* TODO create datastructure that keeps the scripts and names in memory instead of loading files every time*/
		unsigned char* script = NULL;
		long scriptlen;

		/* Build the file path string */
		char* subfolder = "/ta/";
		char* file_ending = encrypted_mode ? ".luata" : ".lua";
		char* script_path = malloc(strlen(app_name)+strlen(subfolder)+strlen(script_name)+strlen(file_ending)+1);
		strcat(script_path, app_name);
		strcat(script_path, subfolder);
		strcat(script_path, script_name);
		strcat(script_path, file_ending);

		if(access(script_path, F_OK ) != -1 ) {
			read_in_file(script_path, &script, &scriptlen);
			invoke_script(script, scriptlen, CALL_MODE_PASS, encrypted_mode, lua_arg, lua_arg_type, lua_ret, &lua_ret_type);
		} else {
			/* If no file with the name was found, fall back on the TA secure storage*/
			invoke_script(script_name, strlen(script_name), CALL_MODE_SAVED, 0, lua_arg, lua_arg_type, lua_ret, &lua_ret_type);
		}
		
		free(script_path);
	}


	stack_from_args(L, lua_ret, lua_ret_type);	

	return 1;  /* number of results */
}


int main(int argc, char *argv[])
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_UUID uuid = TA_LUA_RUNTIME_UUID;
	int opt;
	uint32_t err_origin;

	unsigned char* host_script = NULL;
	long host_scriptlen;

	
	while ((opt = getopt(argc, argv, "us")) != -1) {
        switch (opt) {
        case 'u': encrypted_mode = LUA_MODE_PLAINTEXT; break;
        case 's': call_mode = CALL_MODE_SAVED; break;

        default:
            fprintf(stderr, "Usage: %s [-us] [lua app...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

	app_name = argv[argc-1];


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

	DIR *dir;
	struct dirent *ent;

	char* ta_dir = concat(app_name, "/ta");
	
	if((dir = opendir(ta_dir)) != NULL){

		/* Save all Lua TA scripts (contained in the /ta/ folder) in the TA secure storage to allow for internal calls between the scripts */
		while((ent = readdir(dir)) != NULL){
			
			unsigned char* script = NULL;
			long scriptlen;
			
			/* Build the file name */
			char* file_name = malloc(strlen(ent->d_name) + 1); 
			strcpy(file_name, ent->d_name);
			
			char* script_name = strtok(file_name, ".");
			char* ext = strtok(NULL, ".");
			
			char* ta_dir_slash = concat(ta_dir, "/");
			char* script_path = concat(ta_dir_slash, ent->d_name);
			if(!encrypted_mode && ext && !strcmp(ext,"lua")){
				read_in_file(script_path, &script, &scriptlen);
				save_script(script, scriptlen, LUA_MODE_PLAINTEXT, script_name);
			}else if(encrypted_mode && ext && !strcmp(ext,"luata")){
				read_in_file(script_path, &script, &scriptlen);
				save_script(script,scriptlen, LUA_MODE_ENCRYPTED, script_name);
			}
			
			free(ta_dir_slash);
			free(script);
			free(script_path);
			free(file_name);
		}
		
		closedir (dir);
	} else {
		/* could not open directory */
		perror ("");
		return EXIT_FAILURE;
	}

	free(ta_dir);
	
	/* Load /host/main.lua, the entrypoint for the rich OS Lua script */
	char* main_path = concat(app_name, "/host/main.lua");
	read_in_file(main_path, &host_script, &host_scriptlen);
	free(main_path);

	/* Interpret host lua script TODO: replace with actual standalone interpreter */

	lua_State *L = luaL_newstate();  /* create Lua state */
  	if (L == NULL) {
    	printf("cannot create state: not enough memory");
	}

	luaL_openlibs(L);

	/* Register the function for calling TA Lua scripts with the state */
	lua_pushcfunction(L, TA_call);
    lua_setglobal(L, "TA_call");
	
	/* Load the lua script from the buffer */
	luaL_loadbuffer(L, host_script, host_scriptlen, "lua_script"); 
	
	/* Run the main Lua script */                     
    if (lua_pcall(L, 0, 1, 0))                  
		printf("lua_pcall() failed"); 
	

	/* Return value of operation, for testing purposes */
	char* output = lua_tostring(L, -1);
	printf("%s",output);

    lua_close(L); 
	
	/* Cleanup session and context */
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	free(host_script);

	return 0;
}

