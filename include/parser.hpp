#pragma once

#include<memory>
#include<optional>
#include<span>
#include<string_view>

#include "command.hpp"

struct ParsedFlag {
  Flag flag;
  std::vector<std::string> arguments;
};

struct ParsedInput {
  // Points to the registered command. Does not own it.
  Command *command = nullptr;
  std::vector<ParsedFlag> flags;
  std::vector<std::string> direct_arguments; // Arguments passed directly to the subcommand
                                            // and not to any flag
  std::unique_ptr<ParsedInput> for_subcommand;

  // Searches for a flag with the specified name.
  //
  // Returns the first occurrence, or nullptr if it is not found.
  const ParsedFlag *find_flag(std::string_view name) const;
};

class Parser {
private:
  std::span<Command> commands;
public:
  // Parses the user input.
  //
  // The registered commands must not be destroyed or moved
  // while the parsed input is being used.
  // Returns nullptr for parsing errors.
  std::unique_ptr<ParsedInput> parse_input(std::span<const std::string> args);

  Parser(std::span<Command> commands) : commands(commands) { }
};
