#ifndef PARSER_HPP
#define PARSER_HPP

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
  Command *command = nullptr; // Borrowed from registered command storage.
  std::vector<ParsedFlag> flags;
  std::vector<std::string> direct_arguments; // Arguments passed directly to the subcommand
                                             // and not to any flag
  std::unique_ptr<ParsedInput> for_subcommand;

  // Returns the first occurrence, or nullptr if the flag is absent.
  const ParsedFlag *find_flag(std::string_view name) const;
};

class Parser {
private:
  std::span<Command> commands;
public: 
  // Command storage must remain alive and stable while parsed input is in use.
  std::unique_ptr<ParsedInput> parse_input(std::span<const std::string> args);
  
  Parser(std::span<Command> commands) : commands(commands) { }
};

#endif
