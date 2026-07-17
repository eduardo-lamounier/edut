#include "command.h"

size_t registered_commands_amount;
command_t *commands;

// Helper functions

command_t *command_withname(const char *name) {
  for(size_t i = 0; i < registered_commands_amount; i++)
    if(strcmp(commands[i].name, name) == 0)
      return commands + i;

  return NULL;
}

command_t *get_subcommand_of(command_t command, const char *name) {
  for(size_t i = 0; i < command.subcommands_amount; i++)
    if(strcmp(command.sub_commands[i].name, name) == 0)
      return command.sub_commands + i;

  return NULL;
}

bool find_flag(command_t command, const char *name, size_t *out_idx) {
  for(size_t i = 0; i < command.flags_amount; i++)
    if(strcmp(command.flags[i].text, name) == 0) {
      *out_idx = i;
      return true;
    }

  return false;
}

// Header defined functions

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

void push_lua_command(lua_State *L, command_t *command) {
  lua_newtable(L);

  lua_pushlightuserdata(L, command);
  lua_pushcclosure(L, l_command_execute, 1);
  lua_setfield(L, -2, "execute");

  lua_pushlightuserdata(L, command);
  lua_pushcclosure(L, l_command_getname, 1);
  lua_setfield(L, -2, "get_name");
}

int l_command_execute(lua_State *L) {
  command_t *command =
    (command_t*)lua_touserdata(L, lua_upvalueindex(1));

  lua_rawgeti(L, LUA_REGISTRYINDEX, command->execute_ref);
  lua_pushvalue(L, -2);
  command_execute(L, NULL);
  return 0;
}

int l_command_getname(lua_State *L) {
  command_t *command =
    (command_t*)lua_touserdata(L, lua_upvalueindex(1));

  lua_pushstring(L, command->name);
  return 1;
}

void free_parsed_input(parsed_input_t *parsed_input) {
  while(parsed_input != NULL) {
    parsed_input_t * temp = parsed_input->for_subcommand;
    free(parsed_input);
    parsed_input = temp;
  }
}

// Returns NULL for parsing errors
parsed_input_t *parse_input(char **args, int n) {
  parsed_input_t *parsed_input =
    (parsed_input_t*)calloc(1, sizeof(parsed_input_t));

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

    if(subcommand != NULL) {
      current->for_subcommand =
        (parsed_input_t*)calloc(1, sizeof(parsed_input_t));

      if(current->for_subcommand == NULL) {
        free_parsed_input(parsed_input);
        return NULL;
      }

      current = current->for_subcommand;
      current->command = subcommand;
      continue;
    }

    size_t command_flag_idx;
    
    if(find_flag(*current->command, args[i],&command_flag_idx)) {
      if(current->flags_amount+1 > MAX_FLAGS || strlen(args[i]) > MAX_FLAG_NAME){
        free_parsed_input(parsed_input);
        return NULL;
      }
      
      strncpy(current->flags[current->flags_amount].text, args[i], MAX_FLAG_NAME+1);
      current->flags[current->flags_amount].text[MAX_FLAG_NAME] = '\0';
      current->flags[current->flags_amount].arguments_amount = current->command->flags[command_flag_idx].arguments_amount;
      current->flags_amount++;
      continue;
    }
    
    if(strncmp(args[i], "--", 2) == 0) {
      printf("Invalid flag '%s' passed to command '%s'.\n", args[i], current->command->name);
      free_parsed_input(parsed_input);
      return NULL;
    }

    ssize_t curr_flag_idx = current->flags_amount - 1;

    if((current->flags_amount == 0 ||
      current->flags_arguments_amount[curr_flag_idx]
        == current->flags[curr_flag_idx].arguments_amount)
    ) {
      current->direct_arguments[current->direct_arguments_amount++] = args[i];
      continue;
    }

    if(current->flags_arguments_amount[curr_flag_idx]+1 > MAX_ARGUMENTS) {
      free_parsed_input(parsed_input);
      return NULL;
    }

    current->flags_arguments[curr_flag_idx][current->flags_arguments_amount[curr_flag_idx]] = args[i];
    current->flags_arguments_amount[curr_flag_idx]++;
  }

  if(parsed_input->command == NULL) {
    free_parsed_input(parsed_input);
    return NULL;
  }

  return parsed_input;
}

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

int l_parsedinput_containsflag(lua_State *L) {
  parsed_input_t *parsed_input =
    (parsed_input_t*)lua_touserdata(L, lua_upvalueindex(1));

  const char *flag = luaL_checkstring(L, 1);

  bool contains_flag = false;
  for(size_t i = 0; i < parsed_input->flags_amount; i++)
    if(strcmp(flag, parsed_input->flags[i].text) == 0) {
      contains_flag = true;
      break;
    }

  lua_pushboolean(L, contains_flag);
  return 1;
}

int l_parsedinput_getargument(lua_State *L) {
  parsed_input_t *parsed_input =
    (parsed_input_t*)lua_touserdata(L, lua_upvalueindex(1));

  const char *flag_text;
  int argument_index;

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
    lua_pushstring(L, parsed_input->direct_arguments[argument_index-1]);
    return 1;
  }

  size_t flag_idx;
  bool found_flag = false;
  for(size_t i = 0; i < parsed_input->flags_amount; i++)
    if(strcmp(flag_text, parsed_input->flags[i].text) == 0) {
      flag_idx = i;
      found_flag = true;
      break;
    }

  if(!found_flag)
    return luaL_error(L, "Unknown flag passed for this command."
      "\nIt's possible to check whether the flag exists with 'contains_flag'.");

  flag_t *flag = parsed_input->flags + flag_idx;

  if(argument_index < 1 || (size_t)argument_index > flag->arguments_amount) {
    lua_pushnil(L);
    return 1;
  }

  lua_pushstring(L, parsed_input->flags_arguments[flag_idx][argument_index-1]);
  return 1;
}

