#ifndef PARSER_HPP
#define PARSER_HPP

#include<optional>
#include<span>
#include<string_view>

#include "command.hpp"

typedef struct parsed_input {
  command_t *command;
  std::vector<flag_t> flags;
  std::vector<std::vector<std::string>> flags_arguments;
  std::vector<std::string> direct_arguments; // Arguments passed directly to the subcommand
                                         // and not to any flag
  struct parsed_input *for_subcommand;
} parsed_input_t;

// Command storage must remain alive and stable while parsed input is in use.
parsed_input_t *parse_input(std::span<command_t> commands,
  std::span<const std::string> args);
void free_parsed_input(parsed_input_t *parsed_input);

// Returns the first matching flag's index, or no value if it is absent.
std::optional<size_t> find_flag(std::span<const flag_t> flags, std::string_view name);

#endif
