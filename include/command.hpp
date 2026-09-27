#ifndef COMMAND_HPP
#define COMMAND_HPP

#include<string>
#include<vector>

struct Flag {
  std::string text;
  size_t arguments_amount;
};

struct Command {
  int execute_ref;
  std::vector<Flag> flags;
  std::string name;
  std::vector<Command> sub_commands;
};
