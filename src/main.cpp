#include<stdio.h>
#include<stdlib.h>
#include<string>
#include<vector>

#include "config.hpp"
#include "lua_api.hpp"
#include "parser.hpp"

#define HELP_MESSAGE "edut - Define and run custom CLI commands in one place.\n" \
  "Configure commands, subcommands, flags, and behavior in Lua.\n"               \
  "\n"                                                                           \
  "Usage: edut <command> [arguments]\n"                                          \
  "       edut --help | -h\n"                                                    \
  "       edut --version | -v\n"                                                 \
  "\n"                                                                           \
  "Options:\n"                                                                   \
  "  -h, --help     Show this help and exit.\n"                                  \
  "  -v, --version  Show the application version and exit.\n"                    \
  "\n"                                                                           \
  "Commands are defined in your edut Lua configuration.\n"                       \
  "The bundled configuration provides the scripts command."

#define EDUT_VERSION "1.1.0"

int main(int argc, char **argv) {
  const std::vector<std::string> args(argv + 1, argv + argc);

  // Built-in options must work without loading or executing user configuration.
  if(argc > 1) {
    if(args[0] == "--help" || args[0] == "-h") {
      puts(HELP_MESSAGE);
      return EXIT_SUCCESS;
    }
    if(args[0] == "--version" || args[0] == "-v") {
      puts("edut " EDUT_VERSION);
      return EXIT_SUCCESS;
    }
  }

  // Lua finalizers may use input wrappers: close Lua before freeing their input.
  // Locals are destroyed in reverse declaration order, including on early returns.
  std::unique_ptr<ParsedInput> parsed_input;
  auto state = load_user_configs();
  lua_State *L = state.get();
  if(!state)
    return EXIT_FAILURE;

  if(argc == 1) {
    report_error("No argument passed to the program.");
    return EXIT_FAILURE;
  }

  auto commands = get_registered_commands();
  Parser parser(commands);

  parsed_input = parser.parse_input(args);

  if(!parsed_input) {
    report_error("Error when parsing the input.");
    return EXIT_FAILURE;
  }

  bool success = command_execute(L, parsed_input.get());

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
