#include<iostream>
#include<list>
#include<stdio.h>
#include<stdlib.h>

extern "C" {
  #include<lauxlib.h>
}

#include "lua_api.hpp"

static std::vector<command_t> *commands;

std::span<command_t> get_registered_commands() {
  if(commands == NULL) return {};
  return *commands;
}

static void push_lua_parsedinput(lua_State *L, parsed_input_t *parsed_input);
static int l_command_execute(lua_State *L);
static int l_command_getname(lua_State *L);
static int l_parsedinput_getsubcommand(lua_State *L);
static int l_parsedinput_forsubcommand(lua_State *L);
static int l_parsedinput_containsflag(lua_State *L);
static int l_parsedinput_getargument(lua_State *L);

// A trailing '=' does not distinguish flags when parsing attached values.
static size_t flag_name_length(const std::string& name) {
  size_t len = name.size();
  return len > 0 && name[len - 1] == '=' ? len - 1 : len;
}

// Stores the information of a specific command.
//
// The 'idx' parameter specifies what's the command - within the
// 'commands' table - that's going to be registered.
//
// The 'commands' table must be ALREADY in the stack.
static void register_command(lua_State *L, std::vector<command_t>& commands, int idx) {
  int stack = lua_gettop(L);
  command_t& command = commands[idx];

  lua_rawgeti(L, -1, idx+1);

  lua_rawgeti(L, -1, 1);
  command.name = luaL_checkstring(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, -1, "flags");
  if(lua_istable(L, -1)) {
    lua_pushnil(L);
    while(lua_next(L, -2) != 0) {
      if(command.flags.size() >= MAX_FLAGS)
        luaL_error(L, "Too many flags for command '%s' (maximum %d)",
          command.name.c_str(), MAX_FLAGS);

      size_t flag_idx = command.flags.size();

      command.flags.emplace_back();
      flag_t& flag = command.flags.back();

      if(lua_isnumber(L, -2) && lua_isstring(L, -1)) {
        flag.text = lua_tostring(L, -1);
        flag.arguments_amount = 0;
      } else if(lua_isstring(L, -2) && lua_isnumber(L, -1)) {
        flag.text = lua_tostring(L, -2);
        lua_Integer count = luaL_checkinteger(L, -1);
        if(count < 0 || count > MAX_ARGUMENTS || count != lua_tonumber(L, -1))
          luaL_error(L, "Flag '%s' must accept between 0 and %d arguments",
            flag.text.c_str(), MAX_ARGUMENTS);
        flag.arguments_amount = (size_t)count;
      } else {
        luaL_error(L, "Expected name of a flag, or name of flag (key) and"
          "then its number of arguments (value)");
      }

      const std::string& name = flag.text;
      size_t name_len = flag_name_length(name);
      for(size_t i = 0; i < flag_idx; i++) {
        const std::string& other = command.flags[i].text;
        if(name_len == flag_name_length(other) && name.compare(0, name_len, other, 0, name_len) == 0)
          luaL_error(L, "Ambiguous flag names '%s' and '%s'", name.c_str(), other.c_str());
      }

      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  lua_getfield(L, -1, "subcommands");
  if(lua_istable(L, -1)) {
    lua_len(L, -1);
    size_t subcommands_amount = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if(subcommands_amount > MAX_SUBCOMMANDS)
      luaL_error(L, "Too many subcommands for command '%s' (maximum %d)",
        command.name.c_str(), MAX_SUBCOMMANDS);

    command.sub_commands.resize(subcommands_amount);

    for(size_t j = 0; j < command.sub_commands.size(); j++)
      register_command(L, command.sub_commands, j);
  }
  lua_pop(L, 1);

  lua_getfield(L, -1, "execute");
  luaL_checktype(L, -1, LUA_TFUNCTION);

  command.execute_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_settop(L, stack);
}

// Keep every registration tree alive while Lua may retain command wrappers.
// Storage also survives luaL_error, which can jump out of registration.
static std::list<std::vector<command_t>> command_trees;

// Implementation of the framework's function 'setup'.
//
// Registers all the user-configuration
static int l_setup(lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);

  lua_getfield(L, 1, "commands");

  if(!lua_istable(L, -1))
    return 0;

  lua_len(L, -1);
  size_t new_commands_amount = lua_tointeger(L, -1);
  lua_pop(L, 1);

  command_trees.emplace_back(new_commands_amount);
  auto& new_commands = command_trees.back();

  for(size_t i = 0; i < new_commands_amount; i++)
    register_command(L, new_commands, i);

  // Lua may catch registration errors with pcall. Publish only complete trees.
  commands = &new_commands;

  return 0;
}

// Reports and error message. Does NOT terminate the program.
void report_error(const std::string& msg) {
  std::cout << "\033[31mERROR: " << msg << "\033[m\n";
}

// Implementation of the framework's function 'report'.
//
// Reports an specified error message, but differently
// from 'err', does not terminate the program.
static int l_report(lua_State *L) {
  report_error(luaL_checkstring(L, 1));
  return 0;
}

// Implementation of the framework's function 'err'.
//
// Reports an error message and terminates the program.
//
// Already releases all resources within the Lua State.
static int l_err(lua_State *L) {
  const char *msg = luaL_checkstring(L, 1);
  report_error(msg);
  lua_close(L);
  exit(EXIT_FAILURE);
}

static const struct luaL_Reg edut_api [] = {
    { "setup", l_setup },
    { "report", l_report },
    { "err", l_err },
    {NULL, NULL}
};

// Implementation of 'require("edut")'
//
// Returns the defined API
int lua_require_api(lua_State *L) {
  luaL_newlib(L, edut_api);
  return 1;
}

// Executes a top-level callback and reports an uncaught Lua error.
bool command_execute(lua_State *L, parsed_input_t *parsed_input) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, parsed_input->command->execute_ref);
  push_lua_parsedinput(L, parsed_input);

  if(lua_pcall(L, 1, 0, 0) != LUA_OK) {
    const char *message = lua_tostring(L, -1);
    report_error(message == NULL ? "Lua callback raised a non-string error" : message);
    lua_pop(L, 1);
    return false;
  }
  return true;
}

// Pushes a command into the lua stack.
static void push_lua_command(lua_State *L, command_t *command) {
  lua_newtable(L);

  lua_pushlightuserdata(L, command);
  lua_pushcclosure(L, l_command_execute, 1);
  lua_setfield(L, -2, "execute");

  lua_pushlightuserdata(L, command);
  lua_pushcclosure(L, l_command_getname, 1);
  lua_setfield(L, -2, "get_name");
}

// Implementation of the framework's 'execute' function.
// It executes the user-implemented execution function for
// a command.
//
// The command being executed must ALREADY be in the lua
// stack.
static int l_command_execute(lua_State *L) {
  command_t *command =
    (command_t*)lua_touserdata(L, lua_upvalueindex(1));

  // Supply nil when the wrapper is called without an input argument.
  lua_settop(L, 1);
  lua_rawgeti(L, LUA_REGISTRYINDEX, command->execute_ref);
  lua_pushvalue(L, 1);
  // Let errors reach the caller, including an explicit Lua pcall handler.
  lua_call(L, 1, 0);
  return 0;
}

// Implementation of the framework's 'get_name' function
static int l_command_getname(lua_State *L) {
  command_t *command =
    (command_t*)lua_touserdata(L, lua_upvalueindex(1));

  lua_pushstring(L, command->name.c_str());
  return 1;
}

// Pushes a parsed_input to the lua stack
static void push_lua_parsedinput(lua_State *L, parsed_input_t *parsed_input) {
  lua_newtable(L);

  lua_pushlightuserdata(L, parsed_input);
  lua_pushcclosure(L, l_parsedinput_getsubcommand, 1);
  lua_setfield(L, -2, "get_subcommand");

  lua_pushlightuserdata(L, parsed_input);
  lua_pushcclosure(L, l_parsedinput_forsubcommand, 1);
  lua_setfield(L, -2, "for_subcommand");

  lua_pushlightuserdata(L, parsed_input);
  lua_pushcclosure(L, l_parsedinput_containsflag, 1);
  lua_setfield(L, -2, "contains_flag");

  lua_pushlightuserdata(L, parsed_input);
  lua_pushcclosure(L, l_parsedinput_getargument, 1);
  lua_setfield(L, -2, "get_argument");
}

// Implementation of the framework's 'get_subcommand' function.
// It returns the command's subcommand.
static int l_parsedinput_getsubcommand(lua_State *L) {
  parsed_input_t *parsed_input =
    (parsed_input_t*)lua_touserdata(L, lua_upvalueindex(1));

  if(parsed_input->for_subcommand == NULL
    || parsed_input->for_subcommand->command == NULL) {
    lua_pushnil(L);
    return 1;
  }

  command_t *subcommand = parsed_input->for_subcommand->command;

  push_lua_command(L, subcommand);
  return 1;
}

// Implementation of the framework's 'for_subcommand' function.
// It returns the parsed_input passed to the command's subcommand.
static int l_parsedinput_forsubcommand(lua_State *L) {
  parsed_input_t *parsed_input =
    (parsed_input_t*)lua_touserdata(L, lua_upvalueindex(1));

  if(parsed_input == NULL || parsed_input->for_subcommand == NULL) {
    lua_pushnil(L);
    return 1;
  }

  push_lua_parsedinput(L, parsed_input->for_subcommand);
  return 1;
}

// Implementation of the framework's 'contains_flag' function.
// It returns whether a flag exists within the parsed_input.
static int l_parsedinput_containsflag(lua_State *L) {
  parsed_input_t *parsed_input =
    (parsed_input_t*)lua_touserdata(L, lua_upvalueindex(1));

  const char *flag = luaL_checkstring(L, 1);

  lua_pushboolean(L, find_flag(parsed_input->flags, flag).has_value());
  return 1;
}

// Implementation of the framework's 'get_argument' function.
// It returns an argument, specified by index, in the parsed_input.
static int l_parsedinput_getargument(lua_State *L) {
  parsed_input_t *parsed_input =
    (parsed_input_t*)lua_touserdata(L, lua_upvalueindex(1));

  const std::vector<std::string> *arguments;
  lua_Integer argument_index;

  if(lua_type(L, 1) == LUA_TSTRING) {
    // Borrow Lua strings: luaL_error may jump past C++ destructors.
    const char *flag_text = luaL_checkstring(L, 1);
    argument_index = luaL_checkinteger(L, 2);
    auto flag_idx = find_flag(parsed_input->flags, flag_text);
    if(!flag_idx)
      return luaL_error(L, "Unknown flag passed for this command."
        "\nIt's possible to check whether the flag exists with 'contains_flag'.");
    arguments = &parsed_input->flags_arguments[*flag_idx];
  } else if(lua_type(L, 1) == LUA_TNUMBER) {
    argument_index = luaL_checkinteger(L, 1);
    arguments = &parsed_input->direct_arguments;
  } else
    return luaL_error(L, "Invalid argument passed for function 'get_argument'."
      "\nExpected a string or a number as first parameter.");

  if(argument_index < 1 || (lua_Unsigned)argument_index > arguments->size())
    lua_pushnil(L);
  else
    lua_pushstring(L, (*arguments)[argument_index-1].c_str());
  return 1;
}
