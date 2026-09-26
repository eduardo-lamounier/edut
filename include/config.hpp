#ifndef CONFIG_HPP
#define CONFIG_HPP

#include<memory>

struct lua_State;

struct LuaStateDeleter {
  void operator()(lua_State *state) const;
};

using LuaState = std::unique_ptr<lua_State, LuaStateDeleter>;

// Loads user configuration; the returned owner closes the Lua state.
// Returns nullptr if state creation, configuration discovery or loading fails.
LuaState load_user_configs();

#endif
