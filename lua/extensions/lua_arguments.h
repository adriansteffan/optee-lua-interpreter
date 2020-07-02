#include "lstate.h"

void args_from_stack(lua_State *L, int index, void** lua_arg, int* lua_arg_type);
void stack_from_args(lua_State *L, void* lua_arg, int lua_arg_type);