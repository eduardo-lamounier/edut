#pragma once

#include<memory>

struct lua_State;

struct LuaStateDeleter {
  void operator()(lua_State *state) const;
};

using LuaState = std::unique_ptr<lua_State, LuaStateDeleter>;

// Loads the user configuration.
//
// The returned smart pointer closes the Lua state when destroyed.
// Returns nullptr if the state could not be created or the
// configuration could not be found or loaded.
LuaState load_user_configs();
