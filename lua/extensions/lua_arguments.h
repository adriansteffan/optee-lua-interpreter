#include "lstate.h"

#ifdef TRUSTED_APP_BUILD
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

void params_from_args_ta(void* lua_arg, int lua_arg_type, TEE_Param params[4]);
void args_from_params_ta(void* lua_arg, TEE_Param params[4]);

#else
#include <tee_client_api.h>

void params_from_args_rich(void* lua_arg, int lua_arg_type, TEEC_Parameter params[4]);
void args_from_params_rich(void* lua_arg, TEEC_Parameter params[4]){

#endif


void args_from_stack(lua_State *L, int index, void** lua_arg, int* lua_arg_type);
void stack_from_args(lua_State *L, void* lua_arg, int lua_arg_type);







