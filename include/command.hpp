#ifndef COMMAND_HPP
#define COMMAND_HPP

#include<string>
#include<vector>

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

#endif
