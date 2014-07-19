#ifndef _LUA_LOADER_H
#define _LUA_LOADER_H

#include <string>

extern "C" {
    #include <lua5.2/lua.h>
    #include <lua5.2/lualib.h>
    #include <lua5.2/lauxlib.h>
}

using namespace std;

class LuaLoader {
public:
   LuaLoader();
   ~LuaLoader();

   bool runScript(string filename);
   string getValue(string key);

private:
   lua_State* lua_state;
};

#endif
