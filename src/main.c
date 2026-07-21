#include <linux/limits.h>
#include<stdio.h>
#include<assert.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<lua.h>
#include<lauxlib.h>
#include<lualib.h>

#include "command.h"

// Stores the information of a specific command.
//
// The 'idx' parameter specifies what's the command - within the
// 'commands' table - that's going to be registered.
//
// The 'commands' table must be ALREADY in the stack.
void register_command(lua_State *L, command_t *commands, int idx) {
  int stack = lua_gettop(L);

  lua_rawgeti(L, -1, idx+1);

  lua_rawgeti(L, -1, 1);
  strncpy(commands[idx].name, luaL_checkstring(L, -1), MAX_COMMAND_NAME+1);
  commands[idx].name[MAX_COMMAND_NAME] = '\0';
  lua_pop(L, 1);

  lua_getfield(L, -1, "flags");
  if(!lua_isnil(L, -1) && lua_istable(L, -1)) {
    commands[idx].flags_amount = 0;

    lua_pushnil(L);
    while(lua_next(L, -2) != 0) {
      assert(commands[idx].flags_amount <= MAX_FLAGS);

      size_t flag_idx = commands[idx].flags_amount;

      const char *flag_text;
      size_t flag_arguments_amount;
      
      if(lua_isnumber(L, -2) && lua_isstring(L, -1)) {
        flag_text = lua_tostring(L, -1);
        flag_arguments_amount = 0;

        commands[idx].flags[flag_idx].arguments_amount = 0;
      } else if(lua_isstring(L, -2) && lua_isnumber(L, -1)) {
        flag_text = lua_tostring(L, -2);
        flag_arguments_amount = lua_tointeger(L, -1);
      } else {
        luaL_error(L, "Expected name of a flag, or name of flag (key) and"
          "then its number of arguments (value)");
        exit(1);
      }

      strncpy(commands[idx].flags[flag_idx].text, flag_text, MAX_FLAG_NAME+1);
      commands[idx].flags[flag_idx].text[MAX_FLAG_NAME] = '\0';

      commands[idx].flags[flag_idx].arguments_amount = flag_arguments_amount;

      commands[idx].flags_amount++;
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  commands[idx].subcommands_amount = 0;

  lua_getfield(L, -1, "subcommands");
  if(!lua_isnil(L, -1) && lua_istable(L, -1)) {
    lua_len(L, -1);
    commands[idx].subcommands_amount = lua_tointeger(L, -1);
    lua_pop(L, 1);

    assert(commands[idx].subcommands_amount <= MAX_SUBCOMMANDS);

    commands[idx].sub_commands = (command_t*)malloc(
      sizeof(command_t) * commands[idx].subcommands_amount
    );

    for(size_t j = 0; j < commands[idx].subcommands_amount; j++)
      register_command(L, commands[idx].sub_commands, j);
  }
  lua_pop(L, 1);

  lua_getfield(L, -1, "execute");
  luaL_checktype(L, -1, LUA_TFUNCTION);

  commands[idx].execute_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_settop(L, stack);
}

// Implementation of the framework's function 'setup'.
//
// Registers all the user-configuration
int l_setup(lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);

  lua_getfield(L, 1, "commands");

  if(!lua_istable(L, -1))
    return 0;

  lua_len(L, -1);
  registered_commands_amount = lua_tointeger(L, -1);
  lua_pop(L, 1);

  commands = malloc(sizeof(command_t) * registered_commands_amount);

  for(size_t i = 0; i < registered_commands_amount; i++)
    register_command(L, commands, i);

  return 0;
}

// Implementation of the framework's function 'err'.
//
// Reports an error message and terminates the program.
int l_err(lua_State *L) {
  const char *text = luaL_checkstring(L, 1); 
  printf("\033[31m");
  printf("ERROR: %s", text);
  printf("\033[m\n");
  exit(EXIT_FAILURE);  return 0;
}

static const struct luaL_Reg edut_api [] = {
    {"setup", l_setup},
    {"err", l_err},
    {NULL, NULL} 
};

// Returns 0 if (and only if) the file could be found.
//
// The out parameter 'filepath_out' will store the
// config file's path if the file is found.
int get_user_lua_configs(char *filepath_out) {
  char *home_folder;
  char *xdg_env = getenv("XDG_CONFIG_HOME");
  char config_folder[PATH_MAX];

  if(xdg_env == NULL || strcmp(xdg_env, "") == 0) {
    home_folder = getenv("HOME");

    if(home_folder == NULL || strcmp(home_folder, "") == 0)
      return 1;

    snprintf(config_folder, PATH_MAX, "%s/.config", home_folder);
  } else
    snprintf(config_folder, PATH_MAX, "%s", xdg_env);
  
  snprintf(filepath_out, PATH_MAX, "%s/edut", config_folder);
  return 0;
}

// Implementation of 'require("edut")'
//
// Returns the defined API
int lua_require_api(lua_State *L) {
  luaL_newlib(L, edut_api);
  return 1;
}

// Returns the lua state after the user configs
// are loaded (still needs to be closed)
lua_State *load_user_configs() {
  lua_State *L = luaL_newstate(); 

  char user_configs_folder[PATH_MAX];
  char init_file_path[PATH_MAX];

  if(get_user_lua_configs(user_configs_folder)) {
    puts("Couldn't read your config file.");
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
  const char *package_path = lua_tostring(L, -1);
  lua_pushfstring(L, "%s;%s/lua/?.lua", package_path, user_configs_folder);
  lua_setfield(L, -3, "path");

  lua_pop(L, 2);

  snprintf(init_file_path, PATH_MAX, "%s/init.lua", user_configs_folder);
  if(luaL_dofile(L, init_file_path) != LUA_OK)
    puts(lua_tostring(L, -1));

  return L;
}

int main(int argc, char **argv) {
  lua_State *L;
  if((L = load_user_configs()) == NULL) 
    return EXIT_FAILURE;

  if(argc == 1) {
    puts("ERROR: Nothing passed to the program at all.");
    return EXIT_FAILURE;
  }

  parsed_input_t *parsed_input = parse_input(argv + 1, argc - 1);

  if(parsed_input == NULL) {
    puts("ERROR: Error when parsing the input.");
    return EXIT_FAILURE;
  }

  command_execute(L, parsed_input);
  
  free_parsed_input(parsed_input);
  return EXIT_SUCCESS;
}

