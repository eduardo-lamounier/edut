#include<stdio.h>
#include<memory>

#include "parser.hpp"

// Searches for a command with a specific name.
static Command *find_command(std::span<Command> commands, const std::string& name) {
  for(auto& command : commands)
    if(command.name == name)
      return &command;

  return NULL;
}

const ParsedFlag *ParsedInput::find_flag(std::string_view name) const {
  for(const auto& parsed_flag : flags)
    if(parsed_flag.flag.text == name) return &parsed_flag;

  return nullptr;
}

struct FlagMatch {
  size_t index;
  std::optional<std::string> inline_value;
};

// Preserve the registered flag name while accepting an attached first value.
static std::optional<FlagMatch> find_input_flag(const Command& command,
                                                const std::string& text) {
  for(size_t i = 0; i < command.flags.size(); i++)
    if(command.flags[i].text == text) return FlagMatch{i, std::nullopt};

  size_t prefix = text.find('=');
  if(prefix == std::string::npos) return std::nullopt;

  for(size_t i = 0; i < command.flags.size(); i++) {
    const Flag *flag = &command.flags[i];
    size_t len = flag->text.size();

    if(flag->arguments_amount > 0 &&
        (len == prefix || (len == prefix + 1 && flag->text[prefix] == '=')) &&
        flag->text.compare(0, prefix, text, 0, prefix) == 0) {
      return FlagMatch{i, text.substr(prefix + 1)};
    }
  }
  return std::nullopt;
}

static bool missing_flag_arguments(ParsedInput *input) {
  if(input->flags.empty()) return false;
  const auto& last_flag = input->flags.back();
  return last_flag.arguments.size() < last_flag.flag.arguments_amount;
}

// Parses the user input
//
// Returns nullptr for parsing errors
std::unique_ptr<ParsedInput> Parser::parse_input(std::span<const std::string> args) {
  if(args.empty()) return nullptr;
  Command *command = find_command(commands, args.front());
  if(command == NULL) return nullptr;

  auto parsed_input = std::make_unique<ParsedInput>();

  parsed_input->command = command;
  ParsedInput *current = parsed_input.get();

  for(const auto& arg : args.subspan(1)) {
    Command *subcommand = find_command(current->command->sub_commands, arg);
    auto flag_match = find_input_flag(*current->command, arg);

    if(missing_flag_arguments(current)) {
      if(subcommand != NULL || flag_match || arg.starts_with("--")) {
        printf("Missing arguments for flag '%s'.\n",
          current->flags.back().flag.text.c_str());
        return nullptr;
      }
      current->flags.back().arguments.push_back(arg);
      continue;
    }

    if(subcommand != NULL) {
      current->for_subcommand = std::make_unique<ParsedInput>();

      current = current->for_subcommand.get();
      current->command = subcommand;
      continue;
    }

    if(flag_match) {
      if(current->flags.size() >= MAX_FLAGS) {
        return nullptr;
      }

      current->flags.push_back({current->command->flags[flag_match->index], {}});
      if(flag_match->inline_value.has_value()) {
        current->flags.back().arguments.push_back(*flag_match->inline_value);
      }
      continue;
    }

    if(arg.starts_with("--")) {
      printf("Invalid flag '%s' passed to command '%s'.\n", arg.c_str(), current->command->name.c_str());
      return nullptr;
    }

    if(current->direct_arguments.size() >= MAX_ARGUMENTS) {
      printf("Too many positional arguments for command '%s'.\n", current->command->name.c_str());
      return nullptr;
    }
    current->direct_arguments.push_back(arg);
  }

  if(missing_flag_arguments(current)) {
    printf("Missing arguments for flag '%s'.\n",
      current->flags.back().flag.text.c_str());
    return nullptr;
  }

  return parsed_input;
}
