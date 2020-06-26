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


int invoke_script_number(unsigned char* script, size_t scriptlen, TEEC_Session *sess_ptr, int b_encrypted, int number, int* output){  
	
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


	struct timeval start, end;

    gettimeofday(&start, NULL);

	TEEC_Result res = TEEC_InvokeCommand(sess_ptr, TA_RUN_LUA_SCRIPT_MATH, &op,
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

int invoke_saved_script_number(TEEC_Session *sess_ptr, char* script_name, int number, int* output){  
	
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


	
	struct timeval start, end;

    gettimeofday(&start, NULL);

	TEEC_Result res = TEEC_InvokeCommand(sess_ptr, TA_RUN_SAVED_LUA_SCRIPT_MATH, &op,
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
/*for comparison purposes*/
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

	TEEC_Result res = TEEC_InvokeCommand(sess_ptr, TA_MATH, &op,
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

static int TA_call(lua_State *L) {
	char* script_name = luaL_checkstring(L, 1); 
	int num = luaL_checknumber(L, 2);
	int res;  
	if(call_mode){
		invoke_saved_script_number(&sess, script_name, num, &res);
	}else{
		/* TODO create datastructure that keeps the scripts and names in memory */
		unsigned char* script = NULL;
		long scriptlen;	

		char* subfolder = "/ta/";
		char* file_ending = encrypted_mode ? ".luata" : ".lua"; 

		char* script_path = malloc(strlen(app_name)+strlen(subfolder)+strlen(script_name)+strlen(file_ending)+1);

		strcat(script_path, app_name);
		strcat(script_path, subfolder);
		strcat(script_path, script_name);
		strcat(script_path, file_ending);

		if(access(script_path, F_OK ) != -1 ) {
			read_in_file(script_path, &script, &scriptlen);
			invoke_script_number(script, scriptlen, &sess, encrypted_mode, num, &res);
		} else {
			invoke_saved_script_number(&sess, script_name, num, &res);
		}
		
		free(script_path);
	}
	
	lua_pushnumber(L, res);
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
		while((ent = readdir(dir)) != NULL){

			unsigned char* script = NULL;
			long scriptlen;
			
			char* file_name = malloc(strlen(ent->d_name) + 1); 
			strcpy(file_name, ent->d_name);
			
			char* script_name = strtok(file_name, ".");
			char* ext = strtok(NULL, ".");
			
			
			char* ta_dir_slash = concat(ta_dir, "/");
			char* script_path = concat(ta_dir_slash, ent->d_name);
			if(!encrypted_mode && ext && !strcmp(ext,"lua")){
				read_in_file(script_path, &script, &scriptlen);
				save_script(script, scriptlen, &sess, LUA_MODE_PLAINTEXT, script_name);
			}else if(encrypted_mode && ext && !strcmp(ext,"luata")){
				read_in_file(script_path, &script, &scriptlen);
				save_script(script,scriptlen, &sess, LUA_MODE_ENCRYPTED, script_name);
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


	
	char* main_path = concat(app_name, "/host/main.lua");
	read_in_file(main_path, &host_script, &host_scriptlen);
	free(main_path);

	/* Interpret host lua script TODO: replace with actual standalone interpreter */

	lua_State *L = luaL_newstate();  /* create Lua state */
  	if (L == NULL) {
    	printf("cannot create state: not enough memory");
	}

	luaL_openlibs(L);

	lua_pushcfunction(L, TA_call);
    lua_setglobal(L, "TA_call");
	
	/* Load the lua script from the buffer */
	luaL_loadbuffer(L, host_script, host_scriptlen, "lua_script"); 
	
	                     
    if (lua_pcall(L, 0, 1, 0))                  
		printf("lua_pcall() failed"); 
	
	/* Return value of operation */
	int output = lua_tonumber(L, -1);
	printf("%d",output);

    lua_close(L); 

	

	/* Cleanup session and context */
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	free(host_script);

	return 0;
}

