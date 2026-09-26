#include<iostream>
#include<string>
#include<optional>
#include<vector>

#include<stdio.h>
#include<assert.h>
#include<stdlib.h>
extern "C" {
  #include<lauxlib.h>
  #include<lualib.h>
}

#include "command.h"

#define HELP_MESSAGE "edut - Define and run custom CLI commands in one place.\n" \
  "Configure commands, subcommands, flags, and behavior in Lua.\n"               \
  "\n"                                                                           \
  "Usage: edut <command> [arguments]\n"                                          \
  "       edut --help | -h\n"                                                    \
  "       edut --version | -v\n"                                                 \
  "\n"                                                                           \
  "Options:\n"                                                                   \
  "  -h, --help     Show this help and exit.\n"                                  \
  "  -v, --version  Show the application version and exit.\n"                    \
  "\n"                                                                           \
  "Commands are defined in your edut Lua configuration.\n"                       \
  "The bundled configuration provides the scripts command."

#define EDUT_VERSION "1.0.1"

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
void register_command(lua_State *L, command_t *commands, int idx) {
  int stack = lua_gettop(L);
  commands[idx] = command_t{};

  lua_rawgeti(L, -1, idx+1);

  lua_rawgeti(L, -1, 1);
  commands[idx].name = luaL_checkstring(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, -1, "flags");
  if(!lua_isnil(L, -1) && lua_istable(L, -1)) {
    lua_pushnil(L);
    while(lua_next(L, -2) != 0) {
      if(commands[idx].flags_amount >= MAX_FLAGS)
        luaL_error(L, "Too many flags for command '%s' (maximum %d)",
          commands[idx].name.c_str(), MAX_FLAGS);

      size_t flag_idx = commands[idx].flags_amount;

      std::string& flag_text = commands[idx].flags[flag_idx].text;
      size_t flag_arguments_amount;
      
      if(lua_isnumber(L, -2) && lua_isstring(L, -1)) {
        flag_text = lua_tostring(L, -1);
        flag_arguments_amount = 0;
      } else if(lua_isstring(L, -2) && lua_isnumber(L, -1)) {
        flag_text = lua_tostring(L, -2);
        lua_Integer count = luaL_checkinteger(L, -1);
        if(count < 0 || count > MAX_ARGUMENTS || count != lua_tonumber(L, -1))
          luaL_error(L, "Flag '%s' must accept between 0 and %d arguments",
            flag_text.c_str(), MAX_ARGUMENTS);
        flag_arguments_amount = (size_t)count;
      } else {
        luaL_error(L, "Expected name of a flag, or name of flag (key) and"
          "then its number of arguments (value)");
        exit(1);
      }

      const std::string& name = commands[idx].flags[flag_idx].text;
      size_t name_len = flag_name_length(name);
      for(size_t i = 0; i < flag_idx; i++) {
        const std::string& other = commands[idx].flags[i].text;
        if(name_len == flag_name_length(other) && name.compare(0, name_len, other, 0, name_len) == 0)
          luaL_error(L, "Ambiguous flag names '%s' and '%s'", name.c_str(), other.c_str());
      }

      commands[idx].flags[flag_idx].arguments_amount = flag_arguments_amount;

      commands[idx].flags_amount++;
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  lua_getfield(L, -1, "subcommands");
  if(!lua_isnil(L, -1) && lua_istable(L, -1)) {
    lua_len(L, -1);
    commands[idx].subcommands_amount = lua_tointeger(L, -1);
    lua_pop(L, 1);

    assert(commands[idx].subcommands_amount <= MAX_SUBCOMMANDS);

    commands[idx].sub_commands = new command_t[commands[idx].subcommands_amount];

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
  size_t new_commands_amount = lua_tointeger(L, -1);
  lua_pop(L, 1);

  command_t *new_commands = new command_t[new_commands_amount];
  if(new_commands == NULL && new_commands_amount != 0)
    return luaL_error(L, "Could not allocate commands");

  for(size_t i = 0; i < new_commands_amount; i++)
    register_command(L, new_commands, i);

  // Lua may catch registration errors with pcall. Publish only complete trees.
  commands = new_commands;
  registered_commands_amount = new_commands_amount;

  return 0;
}

// Reports and error message. Does NOT terminate the program.
void report_error(const std::string& msg) {
  printf("\033[31m");
  std::cout << "ERROR: " << msg;
  printf("\033[m\n");
}

// Reports an error message and terminates the program.
void throw_error(const std::string& msg) {
  report_error(msg);
  exit(EXIT_FAILURE);
}

// Implementation of the framework's function 'report'.
//
// Reports an specified error message, but differently
// from 'err', does not terminate the program.
int l_report(lua_State *L) {
  std::string msg(luaL_checkstring(L, 1));
  report_error(msg);
  return 0;
}

// Implementation of the framework's function 'err'.
//
// Reports an error message and terminates the program.
//
// Already releases all resources within the Lua State.
int l_err(lua_State *L) {
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

// Returns the config directory's path if it's found, 
// std::nullopt otherwise.
std::optional<std::string> get_user_lua_configs() {
  std::string configs_path;

#ifdef _WIN32
  {
    const char *applocaldata = getenv("LOCALAPPDATA");

    if(applocaldata == NULL || applocaldata[0] == '\0')
      return std::nullopt;
    
    configs_path = std::string(applocaldata) + "/edut";
    return configs_path;
  }
#endif

  const char *xdg_env = getenv("XDG_CONFIG_HOME");

  if(xdg_env != NULL && xdg_env[0] != '\0') {
    configs_path = std::string(xdg_env) + "/edut";
    return configs_path;
  }

  const char *home_folder = getenv("HOME");

  if(home_folder == NULL || home_folder[0] == '\0')
    return std::nullopt;

  configs_path = std::string(home_folder) + "/.config/edut";

  return configs_path;
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

  auto user_configs_folder = get_user_lua_configs();
  std::string init_file_path;
   
  if(!user_configs_folder.has_value()) {
    puts("Couldn't find your configs folder.");
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
  const std::string package_path(lua_tostring(L, -1));
  lua_pushfstring(L, "%s;%s/lua/?.lua", package_path.c_str(), user_configs_folder.value().c_str());
  lua_setfield(L, -3, "path");

  lua_pop(L, 2);

  init_file_path = *user_configs_folder + "/init.lua";

  if(luaL_dofile(L, init_file_path.c_str()) != LUA_OK) {
    puts(lua_tostring(L, -1));
    lua_close(L);
    return NULL;
  }

  return L;
}

int main(int argc, char **argv) {
  const std::vector<std::string> args(argv + 1, argv + argc);

  // Built-in options must work without loading or executing user configuration.
  if(argc > 1) {
    if(args[0] == "--help" || args[0] == "-h") {
      puts(HELP_MESSAGE);
      return EXIT_SUCCESS;
    }
    if(args[0] == "--version" || args[0] == "-v") {
      puts("edut " EDUT_VERSION);
      return EXIT_SUCCESS;
    }
  }

  lua_State *L;
  if((L = load_user_configs()) == NULL) 
    return EXIT_FAILURE;

  if(argc == 1) {
    lua_close(L);
    throw_error("No argument passed to the program.");
  }

  parsed_input_t *parsed_input = parse_input(args.data(), argc - 1);

  if(parsed_input == NULL) {
    lua_close(L);
    throw_error("Error when parsing the input.");
  }

  command_execute(L, parsed_input);
  
  lua_close(L);
  free_parsed_input(parsed_input);
  return EXIT_SUCCESS;
}
