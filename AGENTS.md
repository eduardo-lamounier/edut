# Project overview

`edut` is a CLI framework written in C++ with Lua configuration. Users define
commands, nested subcommands, flags, and execution callbacks in Lua, then access
them through one executable. The bundled Lua configuration is a small
getting-started example.

## Code layout

- `src/main.cpp`: built-in options and application startup, execution, and shutdown.
- `src/parser.cpp` and `include/parser.hpp`: argument parsing, command/flag lookup,
  and parsed-input structures and cleanup. Parsing has no Lua dependency and
  receives the command collection explicitly.
- `include/command.hpp`: command and flag structures.
- `src/lua_api.cpp` and `include/lua_api.hpp`: the `edut` Lua module, command
  registration and ownership, callback execution, and private Lua wrappers.
- `src/config.cpp` and `include/config.hpp`: configuration discovery, Lua state
  initialization, module search paths, and loading `init.lua`.
- `config/init.lua`: entry point for the user's Lua configuration.
- `config/lua/example.lua`: example commands for getting started.
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
   Nested errors propagate to the Lua caller and may be caught with `pcall`.
   Uncaught callback errors cause a nonzero CLI exit.

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

## Configuration

The intended Unix configuration location is `$XDG_CONFIG_HOME/edut`, falling back
to `$HOME/.config/edut`. Windows support is partial; see the limitations below.
The repository's `config/` directory is a sample configuration to install there.

## Build and validation

Requires CMake, a compiler supporting C++23, and Lua development headers and libraries.

```sh
cmake -S . -B build
cmake --build build
```

The executable is generated in `build/`. Run the Unix CLI regression suite with
`python3 tests/test_cli.py build/edut`. For behavior changes, build and exercise relevant commands with an isolated
configuration via `XDG_CONFIG_HOME`; avoid using or modifying personal configs.

## Known limitations

- Configuration directory names containing Lua search-path separators or
  placeholders (`;` or `?`) are not supported.
- Command trees from replaced or failed registrations remain until process exit.
  API documentation is minimal.

See `docs/known-issues.md` for details on remaining problems. Value-taking flags
accept separate and attached values; Lua lookups use the registered flag name.

## Working conventions

Keep the C++ framework generic and application-specific behavior in Lua. Follow nearby
code style; Lua formatting settings are in `config/.stylua.toml`. When changing
the C++/Lua boundary, check Lua stack balance, registry references, pointer lifetimes,
and argument bounds. Keep command storage stable while parsed input or Lua
wrappers reference it; registration ownership belongs in `lua_api.cpp`. Update the bundled configuration when changing its API.
Treat the limitations above as context, not instructions to fix unrelated issues.
