# AGENTS.md

## Project

`edut` is a C++23 CLI framework configured through Lua. Users define commands,
subcommands, flags, and execution callbacks in Lua and access them through a
single executable.

Keep the C++ framework generic. Application-specific commands and behavior belong
in Lua configuration.

## Architecture

The main components are:

* `src/main.cpp`: CLI startup and built-in options.
* `src/parser.cpp`: argument parsing. It must remain independent of Lua.
* `src/lua_api.cpp`: C++/Lua API boundary, command registration, callbacks, and
  ownership.
* `src/config.cpp`: configuration discovery and Lua initialization.
* `include/command.hpp`: command and flag data structures.
* `config/`: bundled example configuration.
* `tests/test_cli.py`: CLI regression tests.

Command and flag storage must remain stable while parsed commands or Lua wrappers
reference it. Registration ownership belongs in `lua_api.cpp`.

## Important behavior

* `--help`/`-h` and `--version`/`-v` are handled before Lua configuration is loaded.
* Lua registers commands with `api.setup`.
* Parsing a subcommand does **not** automatically execute it. Lua callbacks
  explicitly dispatch to subcommands.
* Uncaught Lua callback errors must result in a nonzero CLI exit.
* Lua wrapper functions use dot syntax, not colon syntax.
* Lua argument indices are one-based.
* Value-taking flags support both separate and attached values.

## Configuration

On Unix, configuration is loaded from:

1. `$XDG_CONFIG_HOME/edut`
2. `$HOME/.config/edut`

The repository's `config/` directory is an example configuration.

Windows support is currently partial. See `docs/known-issues.md`.

## Build and test

Requirements:

* CMake
* C++23 compiler
* Lua development headers and libraries

```sh
cmake -S . -B build
cmake --build build
python3 tests/test_cli.py build/edut
```

When testing configuration-dependent behavior, use an isolated
`XDG_CONFIG_HOME`. Do not modify the user's personal configuration.

## Working rules

* Follow nearby C++ and Lua code style.
* Lua formatting settings are in `config/.stylua.toml`.
* When modifying the C++/Lua boundary, check:

  * Lua stack balance
  * registry references
  * object and pointer lifetimes
  * argument bounds
* Update the bundled example configuration when changing the Lua API.
* Run the CLI regression suite for behavior changes.
* Do not fix unrelated known issues while working on another task.

