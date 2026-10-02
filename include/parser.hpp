#pragma once

#include<memory>
#include<span>
#include<string_view>

#include "command.hpp"

struct ParsedFlag {
  // Borrows a flag from the registered command.
  const Flag *flag = nullptr;
  std::vector<std::string> arguments;
};

struct ParsedCommand {
  // Borrows a registered command and its flags.
  const Command *command = nullptr;
  std::vector<ParsedFlag> flags;
  std::vector<std::string> direct_arguments;

  std::unique_ptr<ParsedCommand> subcommand;

  // Returns the first occurrence of a flag, or nullptr if absent.
  const ParsedFlag *find_flag(std::string_view name) const;
};

class Parser {
private:
  std::span<const Command> commands;
public:
  // Parses the user input.
  //
  // The registered commands and flags must not be destroyed or moved
  // while the parsed command is being used.
  // Returns nullptr for parsing errors.
  std::unique_ptr<ParsedCommand> parse_input(std::span<const std::string> args);

  Parser(std::span<const Command> commands) : commands(commands) { }
};
