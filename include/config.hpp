#ifndef CONFIG_HPP
#define CONFIG_HPP

struct lua_State;

// Loads user configuration; the caller must close the returned Lua state.
// Returns NULL if configuration discovery or loading fails.
lua_State *load_user_configs();

#endif
