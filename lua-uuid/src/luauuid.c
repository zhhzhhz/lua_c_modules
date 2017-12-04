/*
 * luaguid - get guid
 * (c) HZHANG <zhhzhhz@163.com>
 *
 */

#include <lua.h>
#include <lauxlib.h>
#ifdef _WIN32
#include <objbase.h>
#include <stdio.h>
#else
#include <uuid/uuid.h>  
#endif

#define LIB_NAME		"guid"
#define LIB_VERSION		"1.0.0"
#define GUID_LEN		64 

#if LUA_VERSION_NUM < 501
 #error "Unsuported Lua version. You must use Lua >= 5.1"
#endif

#if LUA_VERSION_NUM < 502
 #define luaL_newlib(L, f)  { lua_newtable(L); luaL_register(L, NULL, f); }
 #define lua_rawlen(L, i)   lua_objlen(L, i)
#endif

static int getuuid(char *buf)
{
	if (!buf){
		return 0;
	}
#ifdef _WIN32
	GUID guid;
	if (S_OK == CoCreateGuid(&guid)){
		_snprintf_s(buf, GUID_LEN, GUID_LEN,
			"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2],
			guid.Data4[3], guid.Data4[4], guid.Data4[5],
			guid.Data4[6], guid.Data4[7]);
		return 1;
	}
	else{
		return 0;
	}
#else
	uuid_t uu;
	uuid_generate(uu);
	uuid_unparse(uu, buf);
	return 1;
#endif
}

static int get(lua_State *L) {
	int param_count = lua_gettop(L);
	int idcount = 0;
	if (param_count > 0){
		idcount = luaL_checkinteger(L, 1);
	}
	if (idcount <= 1){
		char buffer[GUID_LEN] = { 0 };
		if (getuuid(buffer)){
			lua_pushstring(L, buffer);
			lua_pushnil(L);
		}
		else {
			lua_pushnil(L);
			lua_pushstring(L, "create guid error");
		}
		return 2;
	}
	else
	{
		lua_newtable(L);
		int seq_ = 0;
		for (int i = 0; i < idcount; i++)
		{
			char buffer[GUID_LEN] = { 0 };
			if (getuuid(buffer)){
				lua_pushnumber(L, seq_++);
				lua_pushstring(L, buffer);
				lua_settable(L, -3);
			}
		}
		lua_pushnil(L);
		return 2;
	}
}

static const luaL_Reg uuid_funcs[] = {
    { "get",   get },
    { NULL, NULL }
};

#ifdef _WIN32
_declspec (dllexport)
#endif
int luaopen_luauuid(lua_State *L) {
	luaL_register(L, "luauuid", uuid_funcs);
    return 1;
}
