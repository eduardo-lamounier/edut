#include<stdio.h>
#include<memory>

#include "parser.hpp"

// Searches for a command with a specific name.
static Command *command_withname(std::span<Command> commands, const std::string& name) {
  for(auto& command : commands)
    if(command.name == name)
      return &command;

  return NULL;
}

// Searches for a subcommand of a specific command.
//
// Returns NULL if the command does not contain a subcommand
// with the specified name.
static Command *get_subcommand_of(Command& command, const std::string& name) {
  for(auto& subcommand : command.sub_commands)
    if(subcommand.name == name) return &subcommand;

  return NULL;
}

// Returns the first matching flag's index, or no value if it is absent.
std::optional<size_t> find_flag(std::span<const Flag> flags,
    std::string_view name) {
  for(size_t i = 0; i < flags.size(); i++)
    if(flags[i].text == name) return i;

  return std::nullopt;
}

// Preserve the registered flag name while accepting an attached first value.
static bool find_input_flag(const Command& command, const std::string& text, size_t *out_idx,
    std::optional<std::string> *inline_value) {
  inline_value->reset();
  if(auto idx = find_flag(command.flags, text)) {
    *out_idx = *idx;
    return true;
  }

  size_t prefix = text.find('=');
  if(prefix == std::string::npos) return false;
  for(size_t i = 0; i < command.flags.size(); i++) {
    const Flag *flag = &command.flags[i];
    size_t len = flag->text.size();
    if(flag->arguments_amount > 0 &&
        (len == prefix || (len == prefix + 1 && flag->text[prefix] == '=')) &&
        flag->text.compare(0, prefix, text, 0, prefix) == 0) {
      *out_idx = i;
      *inline_value = text.substr(prefix + 1);
      return true;
    }
  }
  return false;
}

static bool missing_flag_arguments(ParsedInput *input) {
  if(input->flags.empty()) return false;
  return input->flags_arguments.back().size() < input->flags.back().arguments_amount;
}

void free_parsed_input(ParsedInput *parsed_input) {
  while(parsed_input != NULL) {
    ParsedInput * temp = parsed_input->for_subcommand;
    delete parsed_input;
    parsed_input = temp;
  }
}

// Parses the user input
//
// Returns NULL for parsing errors
ParsedInput *Parser::parse_input(std::span<const std::string> args) {
  if(args.empty()) return NULL;
  Command *command = command_withname(commands, args.front());
  if(command == NULL) return NULL;

  std::unique_ptr<ParsedInput, decltype(&free_parsed_input)> parsed_input(
    new ParsedInput(), free_parsed_input);

  parsed_input->command = command;
  ParsedInput *current = parsed_input.get();

  for(const auto& arg : args.subspan(1)) {
    Command *subcommand = get_subcommand_of(*current->command, arg);
    size_t command_flag_idx;
    std::optional<std::string> inline_value;
    bool is_flag = find_input_flag(*current->command, arg,
      &command_flag_idx, &inline_value);

    if(missing_flag_arguments(current)) {
      if(subcommand != NULL || is_flag || arg.starts_with("--")) {
        printf("Missing arguments for flag '%s'.\n",
          current->flags.back().text.c_str());
        return NULL;
      }
      current->flags_arguments.back().push_back(arg);
      continue;
    }

    if(subcommand != NULL) {
      current->for_subcommand = new ParsedInput();

      current = current->for_subcommand;
      current->command = subcommand;
      continue;
    }

    if(is_flag) {
      if(current->flags.size() >= MAX_FLAGS) {
        return NULL;
      }

      current->flags.push_back(current->command->flags[command_flag_idx]);
      current->flags_arguments.emplace_back();
      if(inline_value.has_value()) {
        current->flags_arguments.back().push_back(*inline_value);
      }
      continue;
    }

    if(arg.starts_with("--")) {
      printf("Invalid flag '%s' passed to command '%s'.\n", arg.c_str(), current->command->name.c_str());
      return NULL;
    }

    if(current->direct_arguments.size() >= MAX_ARGUMENTS) {
      printf("Too many positional arguments for command '%s'.\n", current->command->name.c_str());
      return NULL;
    }
    current->direct_arguments.push_back(arg);
  }

  if(missing_flag_arguments(current)) {
    printf("Missing arguments for flag '%s'.\n",
      current->flags.back().text.c_str());
    return NULL;
  }

  return parsed_input.release();
}
