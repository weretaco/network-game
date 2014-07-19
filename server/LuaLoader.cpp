#include "LuaLoader.h"

#include <iostream>

using namespace std;

LuaLoader::LuaLoader()
{
   // new Lua state
   std::cout << "[C++] Starting Lua state" << std::endl;
   this->lua_state = luaL_newstate();

   // load Lua libraries
   std::cout << "[C++] Loading Lua libraries" << std::endl;
   static const luaL_Reg lualibs[] = 
   {
      {"base", luaopen_base},
      {"io", luaopen_io},
      {NULL, NULL}
   };
   const luaL_Reg *lib = lualibs;
   for(; lib->func != NULL; lib++)
   {
      std::cout << " loading '" << lib->name << "'" << std::endl;
      luaL_requiref(this->lua_state, lib->name, lib->func, 1);
      lua_settop(this->lua_state, 0);
   }
}

LuaLoader::~LuaLoader()
{
}

bool LuaLoader::runScript(string filename)
{
   // load the script
   int status = luaL_loadfile(this->lua_state, filename.c_str());
   cout << " return: " << status << std::endl;

   // run the script with the given arguments
   cout << "[C++] Running script" << std::endl;
   if (status == LUA_OK) {
      lua_pcall(lua_state, 0, LUA_MULTRET, 0);
      return true;
   } else {
      return false;
   }
}

string LuaLoader::getValue(string key) 
{
   lua_getfield(lua_state, 1, key.c_str());
   string str(luaL_checkstring(lua_state, -1));

   return str;
}
