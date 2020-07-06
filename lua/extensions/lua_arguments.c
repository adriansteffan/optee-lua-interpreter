/**
 * Implementations of the functions declared in lua_arguments.h
 * 
 * Adrian Steffan 2020
 */

#include "lua.h"
#include "lprefix.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lstate.h"

#include <string.h>

#include <lua_runtime_ta.h>
#include "lua_arguments.h"

/* Depending on which side this code is used (rich vs ta), different apis need to be called and included */
#ifdef TRUSTED_APP_BUILD
    #include <tee_internal_api.h>
    #include <tee_internal_api_extensions.h>
    #define MALLOC_(size) TEE_Malloc(size, 0)
#else
	#include <tee_client_api.h>
    #define MALLOC_(size) malloc(size)
#endif

/*
*	A lua script to dump arbitrary lua objects to strings containing lua code.
*	This script is included to provide a POC way of sending arbitrary lua objects to and from TAs.
* 	A pure C implementation would be a more performant alternative.
*   There are possible security concerns with sending arguments in code, so this implementation is only meant to demonstrate
*   the possibility of sending over arbitrary data and should not be used in any production environment.
*/

char* DUMPER_SCRIPT = "--[[ DataDumper.lua\n"
			"Copyright (c) 2007 Olivetti-Engineering SA\n"
			"\n"
			"Permission is hereby granted, free of charge, to any person\n"
			"obtaining a copy of this software and associated documentation\n"
			"files (the \"Software\"), to deal in the Software without\n"
			"restriction, including without limitation the rights to use,\n"
			"copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
			"copies of the Software, and to permit persons to whom the\n"
			"Software is furnished to do so, subject to the following\n"
			"conditions:\n"
			"\n"
			"The above copyright notice and this permission notice shall be\n"
			"included in all copies or substantial portions of the Software.\n"
			"\n"
			"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
			"EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES\n"
			"OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n"
			"NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT\n"
			"HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
			"WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING\n"
			"FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
			"OTHER DEALINGS IN THE SOFTWARE.\n"
			"]]\n"
			"\n"
			"arg = ...\n"
			"\n"
			"local dumplua_closure = [[\n"
			"local closures = {}\n"
			"local function closure(t) \n"
			"  closures[#closures+1] = t\n"
			"  t[1] = assert(loadstring(t[1]))\n"
			"  return t[1]\n"
			"end\n"
			"\n"
			"for _,t in pairs(closures) do\n"
			"  for i = 2,#t do \n"
			"    debug.setupvalue(t[1], i-1, t[i]) \n"
			"  end \n"
			"end\n"
			"]]\n"
			"\n"
			"local lua_reserved_keywords = {\n"
			"  'and', 'break', 'do', 'else', 'elseif', 'end', 'false', 'for', \n"
			"  'function', 'if', 'in', 'local', 'nil', 'not', 'or', 'repeat', \n"
			"  'return', 'then', 'true', 'until', 'while' }\n"
			"\n"
			"local function keys(t)\n"
			"  local res = {}\n"
			"  local oktypes = { stringstring = true, numbernumber = true }\n"
			"  local function cmpfct(a,b)\n"
			"    if oktypes[type(a)..type(b)] then\n"
			"      return a < b\n"
			"    else\n"
			"      return type(a) < type(b)\n"
			"    end\n"
			"  end\n"
			"  for k in pairs(t) do\n"
			"    res[#res+1] = k\n"
			"  end\n"
			"  table.sort(res, cmpfct)\n"
			"  return res\n"
			"end\n"
			"\n"
			"local c_functions = {}\n"
			"for _,lib in pairs{'_G', 'string', 'table', 'math', \n"
			"    'io', 'os', 'coroutine', 'package', 'debug'} do\n"
			"  local t = _G[lib] or {}\n"
			"  lib = lib .. \".\"\n"
			"  if lib == \"_G.\" then lib = \"\" end\n"
			"  for k,v in pairs(t) do\n"
			"    if type(v) == 'function' and not pcall(string.dump, v) then\n"
			"      c_functions[v] = lib..k\n"
			"    end\n"
			"  end\n"
			"end\n"
			"\n"
			"function DataDumper(value, varname, fastmode, ident)\n"
			"  local defined, dumplua = {}\n"
			"  -- Local variables for speed optimization\n"
			"  local string_format, type, string_dump, string_rep = \n"
			"        string.format, type, string.dump, string.rep\n"
			"  local tostring, pairs, table_concat = \n"
			"        tostring, pairs, table.concat\n"
			"  local keycache, strvalcache, out, closure_cnt = {}, {}, {}, 0\n"
			"  setmetatable(strvalcache, {__index = function(t,value)\n"
			"    local res = string_format('%q', value)\n"
			"    t[value] = res\n"
			"    return res\n"
			"  end})\n"
			"  local fcts = {\n"
			"    string = function(value) return strvalcache[value] end,\n"
			"    number = function(value) return value end,\n"
			"    boolean = function(value) return tostring(value) end,\n"
			"    ['nil'] = function(value) return 'nil' end,\n"
			"    ['function'] = function(value) \n"
			"      return string_format(\"loadstring(%q)\", string_dump(value)) \n"
			"    end,\n"
			"    userdata = function() error(\"Cannot dump userdata\") end,\n"
			"    thread = function() error(\"Cannot dump threads\") end,\n"
			"  }\n"
			"  local function test_defined(value, path)\n"
			"    if defined[value] then\n"
			"      if path:match(\"^getmetatable.*%)$\") then\n"
			"        out[#out+1] = string_format(\"s%s, %s)\\n\", path:sub(2,-2), defined[value])\n"
			"      else\n"
			"        out[#out+1] = path .. \" = \" .. defined[value] .. \"\\n\"\n"
			"      end\n"
			"      return true\n"
			"    end\n"
			"    defined[value] = path\n"
			"  end\n"
			"  local function make_key(t, key)\n"
			"    local s\n"
			"    if type(key) == 'string' and key:match('^[_%a][_%w]*$') then\n"
			"      s = key .. \"=\"\n"
			"    else\n"
			"      s = \"[\" .. dumplua(key, 0) .. \"]=\"\n"
			"    end\n"
			"    t[key] = s\n"
			"    return s\n"
			"  end\n"
			"  for _,k in ipairs(lua_reserved_keywords) do\n"
			"    keycache[k] = '[\"'..k..'\"] = '\n"
			"  end\n"
			"  if fastmode then \n"
			"    fcts.table = function (value)\n"
			"      -- Table value\n"
			"      local numidx = 1\n"
			"      out[#out+1] = \"{\"\n"
			"      for key,val in pairs(value) do\n"
			"        if key == numidx then\n"
			"          numidx = numidx + 1\n"
			"        else\n"
			"          out[#out+1] = keycache[key]\n"
			"        end\n"
			"        local str = dumplua(val)\n"
			"        out[#out+1] = str..\",\"\n"
			"      end\n"
			"      if string.sub(out[#out], -1) == \",\" then\n"
			"        out[#out] = string.sub(out[#out], 1, -2);\n"
			"      end\n"
			"      out[#out+1] = \"}\"\n"
			"      return \"\" \n"
			"    end\n"
			"  else \n"
			"    fcts.table = function (value, ident, path)\n"
			"      if test_defined(value, path) then return \"nil\" end\n"
			"      -- Table value\n"
			"      local sep, str, numidx, totallen = \" \", {}, 1, 0\n"
			"      local meta, metastr = (debug or getfenv()).getmetatable(value)\n"
			"      if meta then\n"
			"        ident = ident + 1\n"
			"        metastr = dumplua(meta, ident, \"getmetatable(\"..path..\")\")\n"
			"        totallen = totallen + #metastr + 16\n"
			"      end\n"
			"      for _,key in pairs(keys(value)) do\n"
			"        local val = value[key]\n"
			"        local s = \"\"\n"
			"        local subpath = path or \"\"\n"
			"        if key == numidx then\n"
			"          subpath = subpath .. \"[\" .. numidx .. \"]\"\n"
			"          numidx = numidx + 1\n"
			"        else\n"
			"          s = keycache[key]\n"
			"          if not s:match \"^%[\" then subpath = subpath .. \".\" end\n"
			"          subpath = subpath .. s:gsub(\"%s*=%s*$\",\"\")\n"
			"        end\n"
			"        s = s .. dumplua(val, ident+1, subpath)\n"
			"        str[#str+1] = s\n"
			"        totallen = totallen + #s + 2\n"
			"      end\n"
			"      if totallen > 80 then\n"
			"        sep = \"\\n\" .. string_rep(\"  \", ident+1)\n"
			"      end\n"
			"      str = \"{\"..sep..table_concat(str, \",\"..sep)..\" \"..sep:sub(1,-3)..\"}\" \n"
			"      if meta then\n"
			"        sep = sep:sub(1,-3)\n"
			"        return \"setmetatable(\"..sep..str..\",\"..sep..metastr..sep:sub(1,-3)..\")\"\n"
			"      end\n"
			"      return str\n"
			"    end\n"
			"    fcts['function'] = function (value, ident, path)\n"
			"      if test_defined(value, path) then return \"nil\" end\n"
			"      if c_functions[value] then\n"
			"        return c_functions[value]\n"
			"      elseif debug == nil or debug.getupvalue(value, 1) == nil then\n"
			"        return string_format(\"loadstring(%q)\", string_dump(value))\n"
			"      end\n"
			"      closure_cnt = closure_cnt + 1\n"
			"      local res = {string.dump(value)}\n"
			"      for i = 1,math.huge do\n"
			"        local name, v = debug.getupvalue(value,i)\n"
			"        if name == nil then break end\n"
			"        res[i+1] = v\n"
			"      end\n"
			"      return \"closure \" .. dumplua(res, ident, \"closures[\"..closure_cnt..\"]\")\n"
			"    end\n"
			"  end\n"
			"  function dumplua(value, ident, path)\n"
			"    return fcts[type(value)](value, ident, path)\n"
			"  end\n"
			"  if varname == nil then\n"
			"    varname = \"return \"\n"
			"  elseif varname:match(\"^[%a_][%w_]*$\") then\n"
			"    varname = varname .. \" = \"\n"
			"  end\n"
			"  if fastmode then\n"
			"    setmetatable(keycache, {__index = make_key })\n"
			"    out[1] = varname\n"
			"    table.insert(out,dumplua(value, 0))\n"
			"    return table.concat(out)\n"
			"  else\n"
			"    setmetatable(keycache, {__index = make_key })\n"
			"    local items = {}\n"
			"    for i=1,10 do items[i] = '' end\n"
			"    items[3] = dumplua(value, ident or 0, \"t\")\n"
			"    if closure_cnt > 0 then\n"
			"      items[1], items[6] = dumplua_closure:match(\"(.*\\n)\\n(.*)\")\n"
			"      out[#out+1] = \"\"\n"
			"    end\n"
			"    if #out > 0 then\n"
			"      items[2], items[4] = \"local t = \", \"\\n\"\n"
			"      items[5] = table.concat(out)\n"
			"      items[7] = varname .. \"t\"\n"
			"    else\n"
			"      items[2] = varname\n"
			"    end\n"
			"    return table.concat(items)\n"
			"  end\n"
			"end\n"
			"\n"
			"  \n"
			"return(DataDumper(arg,nil,true))";


void stack_from_args(lua_State *L, void* lua_arg, int lua_arg_type){
    
	switch(lua_arg_type){
		case LUA_TYPE_NUMBER:
			lua_pushnumber(L, *(int *)lua_arg);
			break;
		case LUA_TYPE_STRING:
			lua_pushstring(L, *(char**)lua_arg);
			break;
		case LUA_TYPE_CODE: 
			/* The lua data fits no datatype that can be directly represented in c, so deserialization is required*/

			/* lua_arg points to a string containing a lua script, which is to be executed to load the contained data onto the stack*/
			/* load the lua arg script */
			luaL_loadbuffer(L, *(char**)lua_arg, strlen(*(char**)lua_arg), "lua_arg_script"); 

			/* execute the passed script, the lua argument now rests on top of the stack */					
			if (lua_pcall(L, 0, 1, 0))                  
				printf("lua_pcall() failed"); 

			break;
		default:
			break;
	}
}


void args_from_stack(lua_State *L, int index,void** lua_arg, int* lua_arg_type){

	switch(lua_type(L, index)){
		case LUA_TNUMBER:
			*lua_arg_type = LUA_TYPE_NUMBER;
			*lua_arg = (int *) MALLOC_(sizeof(int));
			**(int **)lua_arg = luaL_checknumber(L, index);
			break;

		case LUA_TSTRING:
			*lua_arg_type = LUA_TYPE_STRING; 
			*lua_arg = MALLOC_(sizeof(char*));
            **(char***)lua_arg = luaL_checkstring(L, index);
			break;

		default:
			/*The lua data fits no datatype that can be directly represented in c, so serialization is needed */

			*lua_arg_type = LUA_TYPE_CODE;
			
			/* Note: This is currently broken on the TA side, as the TA runs out of heap memory when loading the dumper script */
			luaL_loadbuffer(L, DUMPER_SCRIPT, strlen(DUMPER_SCRIPT), "lua_arg_script"); 
			

			/* put the object to be dumped on top of the stack*/
			lua_pushvalue(L, index);

			/* dump stack object to a string -> the string contains a lua script directly returning the object*/
			if (lua_pcall(L, 1, 1, 0))                  
				printf("lua_pcall() failed"); 

			
			/* get the lua script form the stack */
			*lua_arg = MALLOC_(sizeof(char*));
            **(char***)lua_arg = luaL_checkstring(L, -1);


			lua_pop(L,-1); // TODO: check if this is needed and/or breaks anything
			break;
	}
}


#ifdef TRUSTED_APP_BUILD


void params_from_args_ta(void* lua_arg, int lua_arg_type, TEE_Param params[4]){

	switch(lua_arg_type){
		case LUA_TYPE_NUMBER:
			params[1].value.b = *(int*)lua_arg;
			break;
		case LUA_TYPE_STRING:
		case LUA_TYPE_CODE:
			/* copy the string into the preallocated buffer (rich side)*/
			strncpy(params[3].memref.buffer, *(char**)lua_arg, BYTE_BUFFER_SIZE);
			params[3].memref.size = strlen(params[3].memref.buffer)+1;
			break;
		default:
			break;
	}
}


void args_from_params_ta(void* lua_arg, TEE_Param params[4]){

	switch(params[1].value.a){
		case LUA_TYPE_NUMBER:
			*(int*)lua_arg = &params[1].value.b;
			break;
		case LUA_TYPE_STRING:
		case LUA_TYPE_CODE:
			*(char**)lua_arg = &params[3].memref.buffer;
			break;
		default:
			break;
	}

}

#else

void params_from_args_rich(void* lua_arg, int lua_arg_type, TEEC_Parameter params[4]){

	switch(lua_arg_type){
		case LUA_TYPE_NUMBER:
			params[1].value.b = *(int*)lua_arg;
			break;
		case LUA_TYPE_STRING:
		case LUA_TYPE_CODE:
			/* copy the string into the preallocated buffer (rich side)*/
			strncpy(params[3].tmpref.buffer, *(char**)lua_arg, BYTE_BUFFER_SIZE);
			break;
		default:
			break;
	}
}

void args_from_params_rich(void* lua_arg, TEEC_Parameter params[4]){

	
	switch(params[1].value.a){
		case LUA_TYPE_NUMBER:
			*(int*)lua_arg = params[1].value.b;
			break;
		case LUA_TYPE_STRING:
		case LUA_TYPE_CODE:
			*(char**)lua_arg = params[3].tmpref.buffer;
			break;
		default:
			break;
	}
}

#endif

