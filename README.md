# edut

A command-line framework that lets users define CLI commands, arguments, and behavior in Lua while relying on a C++ runtime for parsing and execution.

## Installation

The user must have Lua to use the framework, as great part of the Lua configuration is stored and managed using Lua's runtime.

You can install the pre-compiled executable in the repository's releases, but if you wish to install from the source:

Firstly clone it:
```
# Clones the project and enters the created directory
git clone https://github.com/eduardo-lamounier/edut
cd edut
```

Building from source requires CMake, a compiler supporting C++23, and Lua
development headers and libraries.

The project can then be built and compiled with CMake:
```
# Builds the project
cmake -B build 

# Compiles with release optimizations
cmake --build build --config Release
```

The executable will be generated in `build/edut` for you to use it.

You can use the default configs copying its folder to, depending on your OS:

On Linux and MacOS (Unix):
```
mkdir -p ~/.config && cp -r config ~/.config/edut
```

On Windows (Powershell):
```pwsh
cp -Recurse config "$env:LOCALAPPDATA\edut"
```

There you can also make your own changes.

## Built-in options

- `edut --help` or `edut -h`: describe the application and show usage.
- `edut --version` or `edut -v`: print `edut 1.0.1`.

These options exit successfully without loading Lua configuration, so they also
work when configuration is missing or invalid. They apply as the first argument;
flags after a command name are handled by that command's configuration.

## Flag arguments

Flags declared with values in Lua, such as `["--output-file="] = 1`, accept both
`--output-file= file.txt` and `--output-file=file.txt`. The Lua lookup name remains
`--output-file=` in either case. Flags declared without a trailing `=`, such as
`["--output"] = 1`, also accept `--output file.txt` and `--output=file.txt`.

All declared values must be supplied before another flag or subcommand. An
attached value can contain a flag or subcommand name without being interpreted
as one. For flags accepting multiple values, the attached value is the first;
the remaining values are separate arguments. For a name ending in `=`, the exact
token (for example `--output-file=`) retains its original meaning and expects a
separate value; pass an empty quoted argument to supply an empty value.

Flag names within a command must be unique even after removing a trailing `=`.
For example, declaring both `--output` and `--output=` is rejected because
`--output=file.txt` would otherwise be ambiguous.

## Script execution and errors

`scripts run` accepts a direct filename in the configuration's `scripts/`
directory. Nested paths, absolute paths, and symlink scripts are rejected. Spaces,
quotes, and shell metacharacters in script and output filenames are treated
literally.

Foreground execution returns a nonzero CLI status if the script fails or output
capture fails. `--output-file=` appends combined standard output and error while
also displaying them. The CLI reports a failure status rather than forwarding
the script's exact exit code.

`--on-background` checks the script and output destination before launch and
reports only that the job was launched; completion is not tracked. Its standard
streams are detached. Supply `--output-file=` to retain background output.

Uncaught Lua callback errors also produce a nonzero CLI exit. Errors from nested
command callbacks propagate to their caller; Lua can explicitly recover with
`pcall`.

## Development

The C++ code is organized by responsibility:

- `src/main.cpp`: built-in options and application startup/shutdown.
- `src/parser.cpp`: argument parsing and command/flag lookup, independent of Lua.
- `src/lua_api.cpp`: Lua API, registration ownership, wrappers, and execution.
- `src/config.cpp`: configuration discovery and loading.
- `include/command.hpp`: shared command and flag definitions. The other `.hpp`
  headers expose the corresponding modules' interfaces.

Names and arguments use `std::string`, and command and argument collections use
vectors. Command and flag names no longer have a 20-character limit; flag,
subcommand, and argument counts still have explicit limits. Registration trees
remain alive until process exit so retained Lua wrappers keep valid pointers.

After building, run the CLI regression tests on Unix with Python 3:

```sh
python3 tests/test_cli.py build/edut
```

Tests use temporary configurations and disposable scripts; they do not execute
the bundled generators.
See [known issues](docs/known-issues.md) for remaining bugs and unfinished features.

## Collaborating

The framework is simple and certainly needs some improvement, so any help for the project will be very well received! It can be with creating a good documentation, adding support to other languages, adding support for other operating systems etc.
