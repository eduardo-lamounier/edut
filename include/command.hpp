#pragma once

#include<string>
#include<vector>

struct Flag {
  std::string text;
  size_t arguments_amount;
};

struct Command {
  size_t execute_index = 0;
  std::vector<Flag> flags;
  std::string name;
  std::vector<Command> sub_commands;
};
