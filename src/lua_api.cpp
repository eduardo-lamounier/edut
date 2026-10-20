#include<new>
#include<print>
#include<stdio.h>
#include<stdlib.h>

extern "C" {
  #include<lauxlib.h>
}

#include "lua_api.hpp"
#include "config.hpp"

// Points to the current commands, stored in a Lua-owned registration.
static std::vector<Command> *commands;
static lua_State *commands_state;
static int commands_owner_ref = LUA_NOREF;

struct RegisteredCommands {
  std::vector<Command> commands;
  RegisteredCommands *next_to_destroy = nullptr;

  explicit RegisteredCommands(size_t amount) : commands(amount) {}
};

struct RegisteredCommandsHandle {
  RegisteredCommands *registration;
};

static constexpr char REGISTERED_COMMANDS_METATABLE[] = "edut.registered_commands";
static lua_State *closing_state;
static RegisteredCommands *deferred_registrations;

static int l_registered_commands_gc(lua_State *L) {
  auto *handle = static_cast<RegisteredCommandsHandle*>(lua_touserdata(L, 1));
  RegisteredCommands *registration = handle->registration;
  handle->registration = nullptr;

  if(registration == nullptr) return 0;

  if(L == closing_state) {
    registration->next_to_destroy = deferred_registrations;
    deferred_registrations = registration;
  } else {
    delete registration;
  }

  return 0;
}

static void ensure_registered_commands_metatable(lua_State *L) {
  if(luaL_newmetatable(L, REGISTERED_COMMANDS_METATABLE)) {
    lua_pushcfunction(L, l_registered_commands_gc);
    lua_setfield(L, -2, "__gc");
  }
  lua_pop(L, 1);
}

void begin_lua_state_close(lua_State *L) {
  closing_state = L;
}

void finish_lua_state_close(lua_State *L) {
  RegisteredCommands *registration = deferred_registrations;
  while(registration != nullptr) {
    RegisteredCommands *next = registration->next_to_destroy;
    delete registration;
    registration = next;
  }
  deferred_registrations = nullptr;
  closing_state = nullptr;

  if(commands_state == L) {
    commands = nullptr;
    commands_owner_ref = LUA_NOREF;
    commands_state = nullptr;
  }
}

void LuaStateDeleter::operator()(lua_State *state) const {
  if(state == nullptr) return;
  begin_lua_state_close(state);
  lua_close(state);
  finish_lua_state_close(state);
}

std::span<const Command> get_registered_commands() {
  if(commands == NULL) return {};
  return *commands;
}

static void push_lua_parsedcommand(lua_State *L, ParsedCommand *parsed_command,
                                   int owner_index);
static int l_command_execute(lua_State *L);
static int l_command_getname(lua_State *L);
static int l_parsedcommand_getsubcommand(lua_State *L);
static int l_parsedcommand_forsubcommand(lua_State *L);
static int l_parsedcommand_containsflag(lua_State *L);
static int l_parsedcommand_getargument(lua_State *L);

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
static void register_command(lua_State *L, std::vector<Command>& commands, int idx,
                             int owner_index, size_t& execute_index) {
  int stack = lua_gettop(L);
  Command& command = commands[idx];

  lua_rawgeti(L, -1, idx+1);

  lua_rawgeti(L, -1, 1);
  command.name = luaL_checkstring(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, -1, "flags");
  if(lua_istable(L, -1)) {
    lua_pushnil(L);
    while(lua_next(L, -2) != 0) {
      size_t flag_idx = command.flags.size();

      command.flags.emplace_back();
      Flag& flag = command.flags.back();

      if(lua_isnumber(L, -2) && lua_isstring(L, -1)) {
        flag.text = lua_tostring(L, -1);
        flag.arguments_amount = 0;
      } else if(lua_isstring(L, -2) && lua_isnumber(L, -1)) {
        flag.text = lua_tostring(L, -2);
        lua_Integer count = luaL_checkinteger(L, -1);
        if(count < 0 || count != lua_tonumber(L, -1))
          luaL_error(L, "Flag '%s' must accept none or more arguments.",
                     flag.text.c_str());
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

    command.sub_commands.resize(subcommands_amount);

    for(size_t j = 0; j < command.sub_commands.size(); j++)
      register_command(L, command.sub_commands, j, owner_index, execute_index);
  }
  lua_pop(L, 1);

  lua_getfield(L, -1, "execute");
  luaL_checktype(L, -1, LUA_TFUNCTION);

  command.execute_index = ++execute_index;
  lua_getuservalue(L, owner_index);
  lua_pushvalue(L, -2);
  lua_rawseti(L, -2, (lua_Integer)command.execute_index);
  lua_pop(L, 1);

  lua_settop(L, stack);
}

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

  ensure_registered_commands_metatable(L);

  auto *handle = new (lua_newuserdata(L, sizeof(RegisteredCommandsHandle)))
    RegisteredCommandsHandle{nullptr};
  luaL_setmetatable(L, REGISTERED_COMMANDS_METATABLE);
  handle->registration = new RegisteredCommands(new_commands_amount);
  lua_newtable(L);
  lua_setuservalue(L, 3);

  size_t execute_index = 0;
  lua_pushvalue(L, 2);
  for(size_t i = 0; i < new_commands_amount; i++)
    register_command(L, handle->registration->commands, i, 3, execute_index);
  lua_pop(L, 1);

  // Lua may catch registration errors with pcall. Publish only complete trees.
  lua_pushvalue(L, -1);
  const int new_owner_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  if(commands_state == L && commands_owner_ref >= 0)
    luaL_unref(L, LUA_REGISTRYINDEX, commands_owner_ref);
  commands_state = L;
  commands_owner_ref = new_owner_ref;
  commands = &handle->registration->commands;

  return 0;
}

// Reports and error message. Does NOT terminate the program.
void report_error(const std::string& msg) {
  std::println("\033[31mERROR: {}\033[m\n", msg);
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
  begin_lua_state_close(L);
  lua_close(L);
  finish_lua_state_close(L);
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
int l_require_api(lua_State *L) {
  luaL_newlib(L, edut_api);
  return 1;
}

// Executes a top-level callback and reports an uncaught Lua error.
bool command_execute(lua_State *L, ParsedCommand *parsed_command) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, commands_owner_ref);
  const int owner_index = lua_gettop(L);
  lua_getuservalue(L, owner_index);
  lua_rawgeti(L, -1, (lua_Integer)parsed_command->command->execute_index);
  lua_remove(L, -2);
  push_lua_parsedcommand(L, parsed_command, owner_index);
  lua_remove(L, owner_index);

  if(lua_pcall(L, 1, 0, 0) != LUA_OK) {
    const char *message = lua_tostring(L, -1);
    report_error(message == NULL ? "Lua callback raised a non-string error" : message);
    lua_pop(L, 1);
    return false;
  }
  return true;
}

// Pushes a command into the lua stack.
static void push_lua_command(lua_State *L, const Command *command, int owner_index) {
  lua_newtable(L);

  // Lua light userdata stores void*, but command wrappers only read it.
  lua_pushlightuserdata(L, const_cast<Command*>(command));
  lua_pushvalue(L, owner_index);
  lua_pushcclosure(L, l_command_execute, 2);
  lua_setfield(L, -2, "execute");

  lua_pushlightuserdata(L, const_cast<Command*>(command));
  lua_pushvalue(L, owner_index);
  lua_pushcclosure(L, l_command_getname, 2);
  lua_setfield(L, -2, "get_name");
}

// Implementation of the framework's 'execute' function.
// It executes the user-implemented execution function for
// a command.
//
// The command being executed must ALREADY be in the lua
// stack.
static int l_command_execute(lua_State *L) {
  const Command *command =
    (const Command*)lua_touserdata(L, lua_upvalueindex(1));

  // Supply nil when the wrapper is called without an input argument.
  lua_settop(L, 1);
  lua_pushvalue(L, lua_upvalueindex(2));
  lua_getuservalue(L, -1);
  lua_rawgeti(L, -1, (lua_Integer)command->execute_index);
  lua_remove(L, -2);
  lua_remove(L, -2);
  lua_pushvalue(L, 1);
  // Let errors reach the caller, including an explicit Lua pcall handler.
  lua_call(L, 1, 0);
  return 0;
}

// Implementation of the framework's 'get_name' function
static int l_command_getname(lua_State *L) {
  const Command *command =
    (const Command*)lua_touserdata(L, lua_upvalueindex(1));

  lua_pushstring(L, command->name.c_str());
  return 1;
}

// Pushes a parsed command to the lua stack
static void push_lua_parsedcommand(lua_State *L, ParsedCommand *parsed_command,
                                   int owner_index) {
  lua_newtable(L);

  lua_pushlightuserdata(L, parsed_command);
  lua_pushvalue(L, owner_index);
  lua_pushcclosure(L, l_parsedcommand_getsubcommand, 2);
  lua_setfield(L, -2, "get_subcommand");

  lua_pushlightuserdata(L, parsed_command);
  lua_pushvalue(L, owner_index);
  lua_pushcclosure(L, l_parsedcommand_forsubcommand, 2);
  lua_setfield(L, -2, "for_subcommand");

  lua_pushlightuserdata(L, parsed_command);
  lua_pushvalue(L, owner_index);
  lua_pushcclosure(L, l_parsedcommand_containsflag, 2);
  lua_setfield(L, -2, "contains_flag");

  lua_pushlightuserdata(L, parsed_command);
  lua_pushvalue(L, owner_index);
  lua_pushcclosure(L, l_parsedcommand_getargument, 2);
  lua_setfield(L, -2, "get_argument");
}

// Implementation of the framework's 'get_subcommand' function.
// It returns the command's subcommand.
static int l_parsedcommand_getsubcommand(lua_State *L) {
  ParsedCommand *parsed_command =
    (ParsedCommand*)lua_touserdata(L, lua_upvalueindex(1));

  if(parsed_command->subcommand == NULL
    || parsed_command->subcommand->command == NULL) {
    lua_pushnil(L);
    return 1;
  }

  const Command *subcommand = parsed_command->subcommand->command;

  push_lua_command(L, subcommand, lua_upvalueindex(2));
  return 1;
}

// Implementation of the framework's 'for_subcommand' function.
// It returns the parsed command passed to the command's subcommand.
static int l_parsedcommand_forsubcommand(lua_State *L) {
  ParsedCommand *parsed_command =
    (ParsedCommand*)lua_touserdata(L, lua_upvalueindex(1));

  if(parsed_command == NULL || parsed_command->subcommand == NULL) {
    lua_pushnil(L);
    return 1;
  }

  push_lua_parsedcommand(L, parsed_command->subcommand.get(),
                         lua_upvalueindex(2));
  return 1;
}

// Implementation of the framework's 'contains_flag' function.
// It returns whether a flag exists within the parsed command.
static int l_parsedcommand_containsflag(lua_State *L) {
  ParsedCommand *parsed_command =
    (ParsedCommand*)lua_touserdata(L, lua_upvalueindex(1));

  const char *flag = luaL_checkstring(L, 1);

  lua_pushboolean(L, parsed_command->find_flag(flag) != nullptr);
  return 1;
}

// Implementation of the framework's 'get_argument' function.
// It returns an argument, specified by index, in the parsed command.
static int l_parsedcommand_getargument(lua_State *L) {
  ParsedCommand *parsed_command =
    (ParsedCommand*)lua_touserdata(L, lua_upvalueindex(1));

  const std::vector<std::string> *arguments;
  lua_Integer argument_index;

  if(lua_type(L, 1) == LUA_TSTRING) {
    // Borrow Lua strings: luaL_error may jump past C++ destructors.
    const char *flag_text = luaL_checkstring(L, 1);
    argument_index = luaL_checkinteger(L, 2);
    const ParsedFlag *flag = parsed_command->find_flag(flag_text);

    if(flag == nullptr)
      return luaL_error(L, "Unknown flag passed for this command."
        "\nIt's possible to check whether the flag exists with 'contains_flag'.");
    arguments = &flag->arguments;
  } else if(lua_type(L, 1) == LUA_TNUMBER) {
    argument_index = luaL_checkinteger(L, 1);
    arguments = &parsed_command->direct_arguments;
  } else
    return luaL_error(L, "Invalid argument passed for function 'get_argument'."
      "\nExpected a string or a number as first parameter.");

  if(argument_index < 1 || (lua_Unsigned)argument_index > arguments->size())
    lua_pushnil(L);
  else
    lua_pushstring(L, (*arguments)[argument_index-1].c_str());
  return 1;
}
