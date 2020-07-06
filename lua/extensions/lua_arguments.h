/**
 * A collection of funtions that deal with the conversions of 
 * [lua stack element] <-> [argument that can be passed to and from a TA] <-> [TEE parameters to be passed to and from a TA]
 *
 * For all functions dealing with TEE(C) parameters, the following structure is expected:
 * 
 *  params[1].valua.a: Input parameter type specifying what datatype is contained in the params structure (see LUA_TYPE_* below for available types)
 *  params[1].valua.b: (Optional) numerical int argument, gets replaced by numerical return argument
 * 
 *  params[3].(mem/tmp)ref.buffer: A memory buffer allocated on the rich OS side used for transmission of strings and arbitrary elements as lua code. 
 * 	params[3].(mem/tmp)ref.size = The size of the transmitted data (either the size of the buffer or the size of the contained string);
 * 
 *  Adrian Steffan 2020
 */


#include "lstate.h"

#define LUA_TYPE_NUMBER 0   // A integer value
#define LUA_TYPE_STRING 1   // A string value
#define LUA_TYPE_CODE 2     // Any other arbitrary lua element, represented as lua code that return the element

#ifdef TRUSTED_APP_BUILD
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

/**
 * Takes the argument variable and puts the values into the TEE_Param structure for transmission to the rich OS side.
 *
 * @param lua_arg       [in] A pointer to the data that should be put into the param structure  
 * @param lua_arg_type  [in] A flag that tells the fucntion what datatype is contained in lua_arg 
 * @param  params       [in/out] The TEE param structure on the TA side to be filled with the arg value 
 */
void params_from_args_ta(void* lua_arg, int lua_arg_type, TEE_Param params[4]);

/**
 * Extracts the argument from the TEE_Param structure that was received on the TA side.
 *
 * @param lua_arg       [out] A pointer to a pointer, which will point to the extracted value after the function is called
 * @param params        [in/out] The TEE param structure on the TA side filled with the arg value 
 */
void args_from_params_ta(void* lua_arg, TEE_Param params[4]);

#else
#include <tee_client_api.h>


/**
 * Takes the argument variable and puts the values into the TEEC_Param structure for transmission to the TA side.
 *
 * @param lua_arg       [in] A pointer to the data that should be put into the param structure  
 * @param lua_arg_type  [in] A flag that tells the fucntion what datatype is contained in lua_arg 
 * @param params        [in/out] The TEEC param structure on the rich OS side to be filled with the arg value 
 */
void params_from_args_rich(void* lua_arg, int lua_arg_type, TEEC_Parameter params[4]);

/**
 * Extracts the argument from the TEEC_Param structure that was received on the rich OS side.
 *
 * @param lua_arg       [out] A pointer to a pointer, which will point to the extracted value
 * @param params        [in/out] The TEEC param structure on the rich OS side filled with the arg value 
 */
void args_from_params_rich(void* lua_arg, TEEC_Parameter params[4]);

#endif

/**
 * Takes a value from the Lua stack and converts it to a format that can later be attached to TEE(C) Params (either an integer or a char*).
 *
 * @param L             [in/out] A pointer to the Lua stack used
 * @param index         [in] The index of the value on the Lua stack
 * @param lua_arg       [out] A pointer to a void pointer. The pointer will point to the extracted data (either integer or char*). 
 * @param lua_arg_type  [out] A pointer to an integer flag that specifies the datatype contained in lua_arg 
 */
void args_from_stack(lua_State *L, int index, void** lua_arg, int* lua_arg_type);

/**
 * Takes a Lua element in a format that be attached to TEE(C) Params and puts it on top of the Lua stack.
 *
 * @param L             [in/out] A pointer to the Lua stack used
 * @param lua_arg       [in] A pointer to the data that should be put onto the lua stack
 * @param lua_arg_type  [in] An integer flag that specifies the datatype contained in lua_arg
 */
void stack_from_args(lua_State *L, void* lua_arg, int lua_arg_type);







