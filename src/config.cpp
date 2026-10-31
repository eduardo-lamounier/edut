#include<optional>
#include<print>
#include<stdio.h>
#include<stdlib.h>
#include<string>

extern "C" {
  #include<lauxlib.h>
  #include<lualib.h>
}

#include "config.hpp"
#include "lua_api.hpp"

// Returns the config directory's path if it's found,
// std::nullopt otherwise.
static std::optional<std::string> get_user_lua_configs() {
#ifdef _WIN32
  const char *applocaldata = getenv("LOCALAPPDATA");
  if(applocaldata == NULL || applocaldata[0] == '\0') return std::nullopt;
  return std::string(applocaldata) + "/edut";
#else
  const char *xdg_env = getenv("XDG_CONFIG_HOME");
  if(xdg_env != NULL && xdg_env[0] != '\0')
    return std::string(xdg_env) + "/edut";

  const char *home_folder = getenv("HOME");
  if(home_folder == NULL || home_folder[0] == '\0') return std::nullopt;
  return std::string(home_folder) + "/.config/edut";
#endif
}

// Returns the Lua state after the user configs are loaded.
// The returned smart pointer closes it when destroyed.
static int l_search_user_lua_module(lua_State *L);

LuaState load_user_configs() {
  LuaState state(luaL_newstate());
  if(state == nullptr) {
    std::println("Couldn't create the Lua state.");
    return nullptr;
  }
  lua_State *L = state.get();

  std::optional<std::string> user_configs_folder = get_user_lua_configs();

  if(!user_configs_folder.has_value()) {
    std::println("Couldn't find your configs folder.");
    return nullptr;
  }

  luaL_openlibs(L);

  lua_getglobal(L, "package");

  lua_getfield(L, -1, "preload");
  lua_pushcfunction(L, l_require_api);
  lua_setfield(L, -2, "edut");

  lua_pop(L, 1);

#if LUA_VERSION_NUM >= 503
  lua_getfield(L, -1, "searchers");
#else
  lua_getfield(L, -1, "loaders");
#endif
  const lua_Integer searcher_index = 3;
  const size_t searchers_amount = lua_rawlen(L, -1);
  for(size_t i = searchers_amount + 1; i > (size_t)searcher_index; i--) {
    lua_rawgeti(L, -1, (lua_Integer)i - 1);
    lua_rawseti(L, -2, (lua_Integer)i);
  }
  lua_pushlstring(L, user_configs_folder->c_str(), user_configs_folder->size());
  lua_pushcclosure(L, l_search_user_lua_module, 1);
  lua_rawseti(L, -2, searcher_index);

  lua_pop(L, 2);

  std::string init_file_path = *user_configs_folder + "/init.lua";

  if(luaL_dofile(L, init_file_path.c_str()) != LUA_OK) {
    puts(lua_tostring(L, -1));
    return nullptr;
  }

  return state;
}

// Adds a config-directory searcher after Lua's regular source-file searcher.
// It joins paths directly instead of encoding the directory in package.path,
// where semicolons and question marks have special meaning.
static int l_search_user_lua_module(lua_State *L) {
  size_t module_length;
  const char *module = luaL_checklstring(L, 1, &module_length);
  const char *config_folder = lua_tostring(L, lua_upvalueindex(1));

  luaL_Buffer path_buffer;
  luaL_buffinit(L, &path_buffer);
  luaL_addstring(&path_buffer, config_folder);
  luaL_addstring(&path_buffer, "/lua/");
  for(size_t i = 0; i < module_length; i++)
    luaL_addchar(&path_buffer, module[i] == '.' ? '/' : module[i]);
  luaL_addstring(&path_buffer, ".lua");
  luaL_pushresult(&path_buffer);

  const int status = luaL_loadfile(L, lua_tostring(L, -1));
  if(status == LUA_OK) {
    lua_insert(L, -2);
    return 2;
  }

  if(status == LUA_ERRFILE) {
    lua_pushfstring(L, "\n\tno file '%s'", lua_tostring(L, -2));
    lua_remove(L, -2);
    lua_remove(L, -2);
    return 1;
  }

  return lua_error(L);
}
