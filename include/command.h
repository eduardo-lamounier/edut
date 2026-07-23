#ifndef COMMAND_H
#define COMMAND_H

#include <lua.h>
#include <stdbool.h>

#define MAX_FLAGS 20     
#define MAX_SUBCOMMANDS 10
#define MAX_ARGUMENTS 10   

#define MAX_COMMAND_NAME 20
#define MAX_FLAG_NAME 20

typedef struct {
  char text[MAX_FLAG_NAME + 1];
  size_t arguments_amount;
} flag_t;

typedef struct command {
  int execute_ref;
  flag_t flags[MAX_FLAGS];
  char name[MAX_COMMAND_NAME + 1];
  struct command *sub_commands;
  size_t subcommands_amount;
  size_t flags_amount;
} command_t;

typedef struct parsed_input {
  command_t *command;
  flag_t flags[MAX_FLAGS];
  size_t flags_amount;
  char *flags_arguments[MAX_FLAGS][MAX_ARGUMENTS];
  size_t flags_arguments_amount[MAX_FLAGS];
  char *direct_arguments[MAX_ARGUMENTS]; // Arguments passed directly to the subcommand
                                         // and not to any flag
  size_t direct_arguments_amount;
  struct parsed_input *for_subcommand;
} parsed_input_t;

extern size_t registered_commands_amount;
extern command_t *commands;

void command_execute(lua_State *L, parsed_input_t *parsed_input);

void push_lua_command(lua_State *L, command_t *command);

int l_command_execute(lua_State *L);
int l_command_getname(lua_State *L);

void free_parsed_input(parsed_input_t *parsed_input);
parsed_input_t *parse_input(char **args, int n);

void push_lua_parsedinput(lua_State *L, parsed_input_t *parsed_input);
parsed_input_t *pop_lua_parsedinput(lua_State *L);

// Can receive multiple flags to check for, returning whether
// any of them are contained in the parsed input
bool parsedinput_containsflag(parsed_input_t parsed_input, const char *flag);

int l_parsedinput_getsubcommand(lua_State *L);
int l_parsedinput_forsubcommand(lua_State *L);
int l_parsedinput_containsflag(lua_State *L);
int l_parsedinput_getargument(lua_State *L);

#endif
