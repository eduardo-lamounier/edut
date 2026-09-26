# Project overview

`edut` is a CLI framework written in C++ with Lua configuration. Users define
commands, nested subcommands, flags, and execution callbacks in Lua, then access
them through one executable. The bundled Lua configuration is an unfinished shell
script manager built on the framework.

## Code layout

- `src/main.cpp`: built-in options and application startup, execution, and shutdown.
- `src/parser.cpp` and `include/parser.hpp`: argument parsing, command/flag lookup,
  and parsed-input structures and cleanup. Parsing has no Lua dependency and
  receives the command collection explicitly.
- `include/command.hpp`: command and flag structures and registration limits.
- `src/lua_api.cpp` and `include/lua_api.hpp`: the `edut` Lua module, command
  registration and ownership, callback execution, and private Lua wrappers.
- `src/config.cpp` and `include/config.hpp`: configuration discovery, Lua state
  initialization, module search paths, and loading `init.lua`.
- `config/init.lua`: entry point for the user's Lua configuration.
- `config/lua/scripting.lua`: bundled `scripts` command and its subcommands.
- `config/scripts/`: shell scripts available to the bundled script manager.
- `CMakeLists.txt`: executable build and Lua dependency configuration.

## Execution and Lua API

Built-in `--help`/`-h` and `--version`/`-v` options are handled before loading Lua
when passed as the first argument. The version is defined by `EDUT_VERSION` in
`src/main.cpp`. Normal command execution follows this flow:

1. C++ locates and loads the user's `init.lua`, adding the configuration's `lua/`
   directory to Lua's module search path.
2. Lua calls `require "edut"` and `api.setup { commands = { ... } }`.
3. C++ registers the command tree and retains Lua callbacks in the Lua registry.
4. C++ parses CLI arguments into a chain of parsed-input structures.
5. C++ invokes the top-level command's `execute` callback. Lua callbacks explicitly
   dispatch to subcommands; parsing a subcommand does not automatically execute it.

A command definition uses its first array element as its name and supports
`flags`, `subcommands`, and a required `execute` function. Flag lists contain
strings for flags without values or entries such as `["--output-file="] = 1`
for flags that accept a specified number of arguments.

The `edut` module exposes `setup`, `report` (print an error), and `err` (exit with
failure). Parsed input exposes `get_subcommand`, `for_subcommand`, `contains_flag`,
and `get_argument`. Command wrappers expose `execute` and `get_name`. These are
closure-based functions called with dot syntax, not colon syntax. Argument indices
are one-based; `get_argument(index)` reads a positional argument and
`get_argument(flag, index)` reads a flag argument.

## Configuration and script behavior

The intended Unix configuration location is `$XDG_CONFIG_HOME/edut`, falling back
to `$HOME/.config/edut`. Windows support is partial; see the limitations below.
The repository's `config/` directory is a sample configuration to install there.

The script manager resolves scripts from the user's configuration directory and
launches them with Bash. Scripts inherit the caller's working directory, allowing
them to operate on the current project when invoked from elsewhere.

## Build and validation

Requires CMake, a compiler supporting C++23, and Lua development headers and libraries.

```sh
cmake -S . -B build
cmake --build build
```

The executable is generated in `build/`. Run the Unix CLI regression suite with
`python3 tests/test_cli.py build/edut`. For behavior changes, build and exercise relevant commands with an isolated
configuration via `XDG_CONFIG_HOME`; avoid using or modifying personal configs.
The bundled Java generator creates files in its working directory, so run it only
in a disposable directory when testing.

## Known limitations

- `scripts run` is implemented; `add`, `rm`, `list`, `show`, and `edit` are stubs.
- `contains_flag` checks only one name, despite the bundled help code passing two.
- Lua callback errors do not reliably propagate to the process exit status.
  Script command strings lack shell quoting, and pipelines can mask failures.
- Command trees from replaced or failed registrations remain until process exit.
  API documentation is minimal.

See `docs/known-issues.md` for details on remaining problems. Value-taking flags
accept separate and attached values; Lua lookups use the registered flag name.

## Working conventions

Keep the C++ framework generic and script-manager behavior in Lua. Follow nearby
code style; Lua formatting settings are in `config/.stylua.toml`. When changing
the C++/Lua boundary, check Lua stack balance, registry references, pointer lifetimes,
and argument bounds. Keep command storage stable while parsed input or Lua
wrappers reference it; registration ownership belongs in `lua_api.cpp`. Update the bundled configuration when changing its API.
Treat the limitations above as context, not instructions to fix unrelated issues.
