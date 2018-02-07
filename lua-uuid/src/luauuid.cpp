/*
 * luaguid - get guid
 * (c) HZHANG <zhhzhhz@163.com>
 *
 */
extern "C" {
#include <lua.h>
#include <lauxlib.h>
};

#include <stdint.h>
#include <atomic>
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

#if defined _WIN32 || defined WIN32
#include <windows.h>
#include <time.h>
#define EPOCHFILETIME 11644473600000000Ui64
#else
#include <sys/time.h>
#include <unistd.h>
#define EPOCHFILETIME 11644473600000000ULL
#endif
#define   sequenceMask  (-1L ^ (-1L << 12L))  

struct  globle
{
public:
	globle() :last_stamp_(0), work_id_(1010), seq_id_(0) {};
	uint64_t last_stamp_;
	int work_id_;
	std::atomic<uint32_t> seq_id_;
};

uint64_t GetCurrMs()
{
#if defined _WIN32 || defined WIN32
	FILETIME filetime;
	uint64_t time = 0;
	GetSystemTimeAsFileTime(&filetime);

	time |= filetime.dwHighDateTime;
	time <<= 32;
	time |= filetime.dwLowDateTime;

	time /= 10;
	time -= EPOCHFILETIME;
	return time / 1000;
#else
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	uint64_t time = tv.tv_usec;
	time /= 1000;
	time += ((uint64_t)tv.tv_sec * 1000);
	return time;
#endif
}

uint64_t WaitNextMs(uint64_t last_stamp)
{
	uint64_t cur = 0;
	do {
		cur = GetCurrMs();
	} while (cur <= last_stamp);
	return cur;
}

uint64_t GetUniqueID(globle *g_info)
{
	uint64_t  unique_id = 0;
	uint64_t now_time = GetCurrMs();
	unique_id = now_time << 22;
	unique_id |= (g_info->work_id_ & 0x3ff) << 12;

	if (now_time < g_info->last_stamp_)
	{
		return 0;
	}
	if (now_time == g_info->last_stamp_)
	{
		++g_info->seq_id_;
		g_info->seq_id_ = (g_info->seq_id_ & sequenceMask);
		if (g_info->seq_id_ == 0)
		{
			now_time = WaitNextMs(g_info->last_stamp_);
		}
	}
	else
	{
		g_info->seq_id_ = 0;
	}
	g_info->last_stamp_ = now_time;
	unique_id |= g_info->seq_id_;
	return unique_id;
}

//36 char UUID
static int get_system_uuid(char *buf)
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
static int getuuid(lua_State *L) {
	int param_count = lua_gettop(L);
	int idcount = 0;
	if (param_count > 0){
		if ( !lua_isnil(L, 1) ){
			idcount = luaL_checkinteger(L, 1);
		}
	}
	if (idcount <= 1){
		char buffer[GUID_LEN] = { 0 };
		if (get_system_uuid(buffer)){
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
			if (get_system_uuid(buffer)){
				lua_pushnumber(L, seq_++);
				lua_pushstring(L, buffer);
				lua_settable(L, -3);
			}
		}
		lua_pushnil(L);
		return 2;
	}
}

//Snowflake UID
static int get_sfuuid(lua_State *L) {
	int param_count = lua_gettop(L);
	int idcount = 0;
	globle gbinfo;

	//IDÊýÁ¿
	if (param_count > 0) {
		if (!lua_isnil(L, 1)) {
			idcount = luaL_checkinteger(L, 1);
		}
	}
	//»úÆ÷ID
	if (param_count > 1) {
		if (!lua_isnil(L, 2)) {
			gbinfo.work_id_ = luaL_checkinteger(L, 2);
		}
	}
	if (idcount <= 1) {
		lua_pushinteger(L, GetUniqueID(&gbinfo));
		lua_pushnil(L);
		return 2;
	}
	else
	{
		lua_newtable(L);
		int seq_ = 0;
		while (seq_<idcount)
		{
			int64_t id = GetUniqueID(&gbinfo);
			if ( id >0 )
			{
				lua_pushinteger(L, seq_++);
				lua_pushinteger(L, id);
				lua_settable(L, -3);
			}
		}
		lua_pushnil(L);
		return 2;
	}
}

//
static int get_uid64(lua_State *L) {
	lua_pushnil(L);
	lua_pushnil(L);
	return 2;
}


static const luaL_Reg uuid_funcs[] = {
    { "get_uuid",   getuuid },
	{ "get_sfuuid",   get_sfuuid },
	{ "get_uid64",   get_uid64 },
    { NULL, NULL }
};

#ifdef _WIN32
_declspec (dllexport)
#endif
int luaopen_luauuid(lua_State *L) {
	luaL_register(L, "luauuid", uuid_funcs);
    return 1;
}
