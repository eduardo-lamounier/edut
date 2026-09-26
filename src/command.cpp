#include<iostream>
#include<optional>

extern "C" {
  #include<stdlib.h>
  #include<lauxlib.h>
}

#include "command.h"

size_t registered_commands_amount;
command_t *commands;

// Helper functions

// Searches for a command with a specific name.
command_t *command_withname(const std::string& name) {
  for(size_t i = 0; i < registered_commands_amount; i++)
    if(commands[i].name == name)
      return commands + i;

  return NULL;
}

// Searches for a subcommand of a specific command.
//
// Returns NULL if the command does not contain a subcommand
// with the specified name.
command_t *get_subcommand_of(const command_t& command, const std::string& name) {
  for(size_t i = 0; i < command.subcommands_amount; i++)
    if(command.sub_commands[i].name == name)
      return command.sub_commands + i;

  return NULL;
}

// Searches for a flag of a specific command.
//
// The index of the found flag is returned with the 'out_idx' out
// parameter.
//
// Returns `true` if the command contains the specified flag,
// `false` otherwise.
bool find_flag(const command_t& command, const std::string& name, size_t *out_idx) {
  for(size_t i = 0; i < command.flags_amount; i++)
    if(command.flags[i].text == name) {
      *out_idx = i;
      return true;
    }

  return false;
}

// Preserve the registered flag name while accepting an attached first value.
static bool find_input_flag(const command_t& command, const std::string& text, size_t *out_idx,
    std::optional<std::string> *inline_value) {
  inline_value->reset();
  if(find_flag(command, text, out_idx)) return true;

  size_t prefix = text.find('=');
  if(prefix == std::string::npos) return false;
  for(size_t i = 0; i < command.flags_amount; i++) {
    const flag_t *flag = &command.flags[i];
    size_t len = flag->text.size();
    if(flag->arguments_amount > 0 &&
        (len == prefix || (len == prefix + 1 && flag->text[prefix] == '=')) &&
        flag->text.compare(0, prefix, text, 0, prefix) == 0) {
      *out_idx = i;
      *inline_value = text.substr(prefix + 1);
      return true;
    }
  }
  return false;
}

static bool missing_flag_arguments(parsed_input_t *input) {
  if(input->flags_amount == 0) return false;
  size_t idx = input->flags_amount - 1;
  return input->flags_arguments_amount[idx] < input->flags[idx].arguments_amount;
}

// Header defined functions

// Executes a command with the specified parsed_input.
//
// The execution function must ALREADY be in the lua stack.
void command_execute(lua_State *L, parsed_input_t *parsed_input) { 
  if(parsed_input != NULL) { 
    lua_rawgeti(L, LUA_REGISTRYINDEX, parsed_input->command->execute_ref);
    push_lua_parsedinput(L, parsed_input); 
  }

  if(lua_pcall(L, 1, 0, 0) != LUA_OK) {
    printf(
      "ERROR: Lua error when executing command: %s",
      lua_tostring(L, -1)
    );
    lua_pop(L, 1);
  }
}

// Pushes a command into the lua stack.
void push_lua_command(lua_State *L, command_t *command) {
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
int l_command_execute(lua_State *L) {
  command_t *command =
    (command_t*)lua_touserdata(L, lua_upvalueindex(1));

  lua_rawgeti(L, LUA_REGISTRYINDEX, command->execute_ref);
  lua_pushvalue(L, -2);
  command_execute(L, NULL);
  return 0;
}

// Implementation of the framework's 'get_name' function
int l_command_getname(lua_State *L) {
  command_t *command =
    (command_t*)lua_touserdata(L, lua_upvalueindex(1));

  lua_pushstring(L, command->name.c_str());
  return 1;
}

void free_parsed_input(parsed_input_t *parsed_input) {
  while(parsed_input != NULL) {
    parsed_input_t * temp = parsed_input->for_subcommand;
    delete parsed_input;
    parsed_input = temp;
  }
}

// Parses the user input
//
// Returns NULL for parsing errors
parsed_input_t *parse_input(const std::string *args, int n) {
  parsed_input_t *parsed_input = new parsed_input_t();

  if(parsed_input == NULL) return NULL;
 
  // Tracking pointer for the inner-most parsed_input
  // where the subcommands, arguments and flags will be
  // added
  parsed_input_t *current = parsed_input;

  for(int i = 0; i < n; i++) { 
    if(parsed_input->command == NULL) {
      command_t *command = command_withname(args[i]);
      
      if(command == NULL) {
        free_parsed_input(parsed_input);
        return NULL;
      }

      current->command = command;
      continue;
    }

    command_t *subcommand = get_subcommand_of(*current->command, args[i]);
    size_t command_flag_idx;
    std::optional<std::string> inline_value;
    bool is_flag = find_input_flag(*current->command, args[i],
      &command_flag_idx, &inline_value);

    if(missing_flag_arguments(current)) {
      if(subcommand != NULL || is_flag || args[i].starts_with("--")) {
        printf("Missing arguments for flag '%s'.\n",
          current->flags[current->flags_amount - 1].text.c_str());
        free_parsed_input(parsed_input);
        return NULL;
      }
      size_t idx = current->flags_amount - 1;
      current->flags_arguments[idx][current->flags_arguments_amount[idx]++] = args[i];
      continue;
    }

    if(subcommand != NULL) {
      current->for_subcommand = new parsed_input_t();

      if(current->for_subcommand == NULL) {
        free_parsed_input(parsed_input);
        return NULL;
      }

      current = current->for_subcommand;
      current->command = subcommand;
      continue;
    }

    if(is_flag) {
      if(current->flags_amount >= MAX_FLAGS) {
        free_parsed_input(parsed_input);
        return NULL;
      }
      
      size_t idx = current->flags_amount;
      current->flags[idx] = current->command->flags[command_flag_idx];
      if(inline_value.has_value()) {
        current->flags_arguments[idx][0] = *inline_value;
        current->flags_arguments_amount[idx] = 1;
      }
      current->flags_amount++;
      continue;
    }
    
    if(args[i].starts_with("--")) {
      printf("Invalid flag '%s' passed to command '%s'.\n", args[i].c_str(), current->command->name.c_str());
      free_parsed_input(parsed_input);
      return NULL;
    }

    if(current->direct_arguments_amount >= MAX_ARGUMENTS) {
      printf("Too many positional arguments for command '%s'.\n", current->command->name.c_str());
      free_parsed_input(parsed_input);
      return NULL;
    }
    current->direct_arguments[current->direct_arguments_amount++] = args[i];
  }

  if(parsed_input->command == NULL) {
    free_parsed_input(parsed_input);
    return NULL;
  }

  if(missing_flag_arguments(current)) {
    printf("Missing arguments for flag '%s'.\n",
      current->flags[current->flags_amount - 1].text.c_str());
    free_parsed_input(parsed_input);
    return NULL;
  }

  return parsed_input;
}

// Pushes a parsed_input to the lua stack
void push_lua_parsedinput(lua_State *L, parsed_input_t *parsed_input) {
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
int l_parsedinput_getsubcommand(lua_State *L) {
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
int l_parsedinput_forsubcommand(lua_State *L) {
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
int l_parsedinput_containsflag(lua_State *L) {
  parsed_input_t *parsed_input =
    (parsed_input_t*)lua_touserdata(L, lua_upvalueindex(1));

  const char *flag = luaL_checkstring(L, 1);

  bool contains_flag = false;
  for(size_t i = 0; i < parsed_input->flags_amount; i++)
    if(parsed_input->flags[i].text == flag) {
      contains_flag = true;
      break;
    }

  lua_pushboolean(L, contains_flag);
  return 1;
}

// Implementation of the framework's 'get_argument' function.
// It returns an argument, specified by index, in the parsed_input.
int l_parsedinput_getargument(lua_State *L) {
  parsed_input_t *parsed_input =
    (parsed_input_t*)lua_touserdata(L, lua_upvalueindex(1));

  // Borrow Lua strings: luaL_error may jump past C++ destructors.
  const char *flag_text;
  lua_Integer argument_index;

  if(lua_type(L, 1) == LUA_TSTRING) {
    flag_text = luaL_checkstring(L, 1);
    argument_index = luaL_checkinteger(L, 2);
  } else if(lua_type(L, 1) == LUA_TNUMBER) {
    flag_text = NULL;
    argument_index = luaL_checkinteger(L, 1);
  } else
    return luaL_error(L, "Invalid argument passed for function 'get_argument'."
      "\nExpected a string or a number as first parameter.");

  if(flag_text == NULL) {
    if(argument_index < 1 ||
        (lua_Unsigned)argument_index > parsed_input->direct_arguments_amount) {
      lua_pushnil(L);
      return 1;
    }
    lua_pushstring(L, parsed_input->direct_arguments[argument_index-1].c_str());
    return 1;
  }

  size_t flag_idx;
  bool found_flag = false;
  for(size_t i = 0; i < parsed_input->flags_amount; i++)
    if(parsed_input->flags[i].text == flag_text) {
      flag_idx = i;
      found_flag = true;
      break;
    }

  if(!found_flag)
    return luaL_error(L, "Unknown flag passed for this command."
      "\nIt's possible to check whether the flag exists with 'contains_flag'.");

  flag_t *flag = parsed_input->flags + flag_idx;

  if(argument_index < 1 || (lua_Unsigned)argument_index > flag->arguments_amount) {
    lua_pushnil(L);
    return 1;
  }

  lua_pushstring(L, parsed_input->flags_arguments[flag_idx][argument_index-1].c_str());
  return 1;
}
