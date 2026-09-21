# edut

A command-line framework that lets users define CLI commands, arguments, and behavior in Lua while relying on a C runtime for parsing and execution.

## Installation

The user must have Lua to use the framework, as great part of the Lua configuration is stored and managed using Lua's runtime.

You can install the pre-compiled executable in the repository's releases, but if you wish to install from the source:

Firstly clone it:
```
# Clones the project and enters the created directory
git clone https://github.com/eduardo-lamounier/edut
cd edut
```

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

## Development

After building, run the CLI regression tests on Unix with Python 3:

```sh
python3 tests/test_cli.py build/edut
```

Tests use temporary configurations and do not execute the bundled scripts.
See [known issues](docs/known-issues.md) for remaining bugs and unfinished features.

## Collaborating

The framework is simple and certainly needs some improvement, so any help for the project will be very well received! It can be with creating a good documentation, adding support to other languages, adding support for other operating systems etc.
