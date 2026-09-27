# Development guide

[Back to the README](../README.md)

## Building

Requires CMake, a compiler supporting C++23, and Lua development headers and
libraries. From the repository root:

```sh
cmake -S . -B build
cmake --build build
```

The executable is generated in `build/`. For installation and a first command,
see the [README](../README.md).

## Source layout

- `src/main.cpp`: built-in options and application startup and shutdown.
- `src/parser.cpp` and `include/parser.hpp`: argument parsing, command and flag
  lookup, and parsed input. Parsing does not depend on Lua.
- `src/lua_api.cpp` and `include/lua_api.hpp`: command registration, Lua wrappers,
  and callback execution.
- `src/config.cpp` and `include/config.hpp`: configuration discovery and loading.
- `include/command.hpp`: command and flag definitions.
- `config/`: sample Lua configuration for getting started.
- `tests/test_cli.py`: Unix CLI regression tests.

## Ownership and execution

Names and arguments use `std::string`, and collections use vectors.
`Parser` receives the registered commands and returns a `std::unique_ptr<ParsedInput>`.
Each parsed input owns its subcommand input and stores each flag with its arguments.
Pointers to registered commands do not own those commands.

Registration trees remain alive until process exit so command wrappers retained
by Lua keep valid pointers. Failed registrations also retain their partial trees;
only complete registrations become the current command collection.

The Lua state is closed by its smart pointer. Parsed input must remain alive
until after Lua closes, because finalizers can still call input wrappers.
Lua errors may skip C++ destructors inside callbacks, so avoid placing local
resource owners across calls that can raise a Lua error. The `api.err` function
exits directly and does not run local C++ destructors.

The top-level callback runs after parsing. Lua callbacks explicitly dispatch to
subcommands, and uncaught callback errors cause a nonzero CLI exit.

## Testing

After building, run the regression suite with Python 3:

```sh
python3 tests/test_cli.py build/edut
```

Tests use temporary configurations and do not modify personal configurations.
For manual checks, set `XDG_CONFIG_HOME` to a temporary directory containing an
`edut/init.lua` configuration.

## Making changes

Keep the C++ framework generic and application-specific behavior in Lua. Follow the
nearby code and comment style; Lua formatting settings are in
`config/.stylua.toml`.

When changing the C++/Lua boundary, check stack balance, registry references,
pointer lifetimes, and argument bounds. Keep command storage stable while parsed
input or Lua wrappers refer to it. Update the sample configuration and user guide
when changing the Lua API.

See [known issues](known-issues.md) for remaining bugs and unfinished features.
