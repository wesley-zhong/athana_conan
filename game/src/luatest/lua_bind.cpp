//extern "C" {
//#include "lua/lua.h"
//#include "lua/lauxlib.h"
//#include "lua/lualib.h"
//}

#include <stdio.h>
#include <stdlib.h>
#include "core/log/XLog.h"
#include "sol/sol.hpp"
//#include "pb.h"


#include <map>

extern void luabind_netserver(sol::state & lua);
extern void luabind_kcpserver(sol::state & lua);
extern void luabind_websocket(sol::state & lua);
extern void luabind_eventloop(sol::state & lua);
extern void luabind_mysql(sol::state & lua);
extern void luabind_redis(sol::state & lua);
extern void luabind_basepacket(sol::state & lua);
extern void luabind_common(sol::state & lua);
extern void luabind_csvpar(sol::state & lua);
extern void luabind_json(sol::state & lua);
extern void luabind_httpserver(sol::state & lua);


void init_lua_pb(lua_State* L)
{
	// luaL_requiref(L, "pb", luaopen_pb, 0);
	// luaL_requiref(L, "pb.slice", luaopen_pb_slice, 0);
	// luaL_requiref(L, "pb.buffer", luaopen_pb_buffer, 0);
	// luaL_requiref(L, "pb.conv", luaopen_pb_conv, 0);
}

int lua_bind(const std::string& lua_file_dir)
{

	sol::state lua;
	lua.open_libraries();
//
//	// bind c++ && lua
//	luabind_netserver(lua);
//	luabind_kcpserver(lua);
//	luabind_websocket(lua);
//	luabind_eventloop(lua);
//	luabind_mysql(lua);
	luabind_redis(lua);
//	luabind_basepacket(lua);
//	luabind_common(lua);
//	luabind_csvpar(lua);
//	luabind_json(lua);
//	luabind_httpserver(lua);
//
	lua.script_file(lua_file_dir.c_str());
//	init_lua_pb(lua.lua_state());

	if (luaL_dofile(lua.lua_state(),lua_file_dir.c_str()) == 1) {
		if (lua_isstring(lua.lua_state(), -1)) {
			printf("load lua file error:%s\n", lua_tostring(lua.lua_state(), -1));
		}
	}

    // auto p = std::make_shared<int>(1);
    // INFO_LOG("HAHA ={}",*p);

//	system("pause");
	return 0;

}