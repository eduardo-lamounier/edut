#ifndef PARSER_HPP
#define PARSER_HPP

#include<memory>
#include<optional>
#include<span>
#include<string_view>

#include "command.hpp"

struct ParsedInput {
  Command *command = nullptr; // Borrowed from registered command storage.
  std::vector<Flag> flags;
  std::vector<std::vector<std::string>> flags_arguments;
  std::vector<std::string> direct_arguments; // Arguments passed directly to the subcommand
                                             // and not to any flag
  std::unique_ptr<ParsedInput> for_subcommand;
};

class Parser {
private:
  std::span<Command> commands;
public: 
  // Command storage must remain alive and stable while parsed input is in use.
  std::unique_ptr<ParsedInput> parse_input(std::span<const std::string> args);
  
  Parser(std::span<Command> commands) : commands(commands) { }
};

// Returns the first matching flag's index, or no value if it is absent.
std::optional<size_t> find_flag(std::span<const Flag> flags, std::string_view name);

#endif
