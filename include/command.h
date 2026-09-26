#ifndef COMMAND_H
#define COMMAND_H

#include<string>
#include<vector>
#include<span>

extern "C" {
  #include <lua.h>
}

#define MAX_FLAGS 20     
#define MAX_SUBCOMMANDS 10
#define MAX_ARGUMENTS 10   


typedef struct {
  std::string text;
  size_t arguments_amount;
} flag_t;

typedef struct command {
  int execute_ref;
  std::vector<flag_t> flags;
  std::string name;
  std::vector<command> sub_commands;
} command_t;

typedef struct parsed_input {
  command_t *command;
  std::vector<flag_t> flags;
  std::vector<std::vector<std::string>> flags_arguments;
  std::vector<std::string> direct_arguments; // Arguments passed directly to the subcommand
                                         // and not to any flag
  struct parsed_input *for_subcommand;
} parsed_input_t;

extern std::vector<command_t> *commands;

void command_execute(lua_State *L, parsed_input_t *parsed_input);

void push_lua_command(lua_State *L, command_t *command);

int l_command_execute(lua_State *L);
int l_command_getname(lua_State *L);

void free_parsed_input(parsed_input_t *parsed_input);
parsed_input_t *parse_input(std::span<const std::string> args);

void push_lua_parsedinput(lua_State *L, parsed_input_t *parsed_input);
int l_parsedinput_getsubcommand(lua_State *L);
int l_parsedinput_forsubcommand(lua_State *L);
int l_parsedinput_containsflag(lua_State *L);
int l_parsedinput_getargument(lua_State *L);

#endif
