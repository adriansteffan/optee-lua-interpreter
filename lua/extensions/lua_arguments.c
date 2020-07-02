#include "lua.h"
#include "lprefix.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lstate.h"

#include <lua_runtime_ta.h>

#ifdef TRUSTED_APP_BUILD
    #include <tee_internal_api.h>
    #include <tee_internal_api_extensions.h>
    #define MALLOC_(size) TEE_Malloc(size, 0)
#else
    #define MALLOC_(size) malloc(size)
#endif


void args_from_stack(lua_State *L, int index,void** lua_arg, int* lua_arg_type){
    
	
	switch(lua_type(L, index)){
		case LUA_TNUMBER:
			*lua_arg = (int *) MALLOC_(sizeof(int));
			**(int **)lua_arg = luaL_checknumber(L, index);
			*lua_arg_type = LUA_TYPE_NUMBER;
			break;
		case LUA_TSTRING:
			*lua_arg = MALLOC_(sizeof(char*));
            **(char***)lua_arg = luaL_checkstring(L, index);
			*lua_arg_type = LUA_TYPE_STRING; 
			break;
		default:
			// dump the string
			*lua_arg_type = LUA_TYPE_CODE;
			break;
	}
}

void stack_from_args(lua_State *L, void* lua_arg, int lua_arg_type){
    
	switch(lua_arg_type){
		case LUA_TYPE_NUMBER:
			lua_pushnumber(L, *(int *)lua_arg);
			break;
		case LUA_TYPE_STRING:
			lua_pushstring(L, *(char**)lua_arg);
			break;
		case LUA_TYPE_CODE:
			// execute dump
			break;
		default:
			printf("Invalid output type supplied to invoke_script");
			return 1;
	}
}
