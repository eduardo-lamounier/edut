#include<optional>
#include<stdio.h>
#include<stdlib.h>

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

// Returns the lua state after the user configs
// are loaded (still needs to be closed)
lua_State *load_user_configs() {
  lua_State *L = luaL_newstate();

  auto user_configs_folder = get_user_lua_configs();

  if(!user_configs_folder.has_value()) {
    puts("Couldn't find your configs folder.");
    lua_close(L);
    return NULL;
  }

  luaL_openlibs(L);

  lua_getglobal(L, "package");

  lua_getfield(L, -1, "preload");
  lua_pushcfunction(L, lua_require_api);
  lua_setfield(L, -2, "edut");

  lua_pop(L, 1);

  lua_getfield(L, -1, "path");
  const std::string package_path(lua_tostring(L, -1));
  lua_pushfstring(L, "%s;%s/lua/?.lua", package_path.c_str(), user_configs_folder.value().c_str());
  lua_setfield(L, -3, "path");

  lua_pop(L, 2);

  std::string init_file_path = *user_configs_folder + "/init.lua";

  if(luaL_dofile(L, init_file_path.c_str()) != LUA_OK) {
    puts(lua_tostring(L, -1));
    lua_close(L);
    return NULL;
  }

  return L;
}
