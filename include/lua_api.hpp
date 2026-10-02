#pragma once

extern "C" {
  #include<lua.h>
}

#include "parser.hpp"

int l_require_api(lua_State *L);
std::span<const Command> get_registered_commands();
bool command_execute(lua_State *L, ParsedCommand *parsed_command);
void report_error(const std::string& msg);
