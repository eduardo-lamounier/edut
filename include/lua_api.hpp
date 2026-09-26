#ifndef LUA_API_HPP
#define LUA_API_HPP

extern "C" {
  #include<lua.h>
}

#include "parser.hpp"

int lua_require_api(lua_State *L);
std::span<Command> get_registered_commands();
bool command_execute(lua_State *L, ParsedInput *parsed_input);
void report_error(const std::string& msg);

#endif
