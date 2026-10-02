# edut

Define your own command-line commands in Lua and run them through `edut`.
You can add arguments, flags, and subcommands, then choose what each command does.
The sample configuration provides a small example to help you get started.

- **Learning edut?** Follow the [configuration tutorial](docs/configuration.md)
  from your first command through flags, subcommands, and modules.
- **Looking up a function?** Open the [Lua API reference](docs/api.md#function-index)
  for signatures, return values, errors, and examples.

## Installation

Building from source requires CMake, a compiler supporting C++23, and Lua
development headers and libraries.

```sh
git clone https://github.com/eduardo-lamounier/edut
cd edut
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

On Unix, the executable is generated at `build/edut`. You can run it from there,
or copy it to a directory in your `PATH` to use `edut` from any directory.
The examples below assume it is in your `PATH`.

Windows support is partial. The configuration loader recognizes
`%LOCALAPPDATA%/edut`; native Windows builds still need validation.
See [platform limitations](docs/known-issues.md#platform-support).

## Your first command

On Unix, `edut` reads `init.lua` from `$XDG_CONFIG_HOME/edut`, or
`~/.config/edut` when `XDG_CONFIG_HOME` is unset or empty. Create that directory:

```sh
mkdir -p "${XDG_CONFIG_HOME:-$HOME/.config}/edut"
```

For a new configuration, create `init.lua` inside it with this command:

```lua
local api = require "edut"

api.setup {
  commands = {
    {
      "hello",
      execute = function(input)
        local name = input.get_argument(1) or "world"
        print("Hello, " .. name .. "!")
      end,
    },
  },
}
```

Then run:

```sh
edut hello
edut hello someone
```

The commands print `Hello, world!` and `Hello, someone!` respectively.
To add flags or organize commands into modules, see the
[configuration guide](docs/configuration.md).

## Using the sample configuration

For a new installation, copy the contents of `config/` into your configuration
directory:

```sh
mkdir -p "${XDG_CONFIG_HOME:-$HOME/.config}/edut"
cp -r config/. "${XDG_CONFIG_HOME:-$HOME/.config}/edut/"
```

If you already have a configuration, merge the files you want instead of replacing
it. The sample loads [lua/example.lua](config/lua/example.lua), which demonstrates
subcommands, positional arguments, and flags:

```sh
edut example --help
edut example greet
edut example greet Someone --greeting=Hi --upper
```

The greetings print `Hello, world!` and `HI, SOMEONE!`. Edit or replace the example
module with your own commands and list your modules in `init.lua`.

## Built-in options

- `edut --help` or `edut -h`: show usage.
- `edut --version` or `edut -v`: print the installed version.

These options work without loading your configuration. They apply when passed
as the first argument; flags after a command name are handled by that command.

## More documentation

- [Configuration tutorial](docs/configuration.md): build commands step by step.
- [Lua API reference](docs/api.md): function index, definition tables, and parsing rules.
- [Known issues](docs/known-issues.md): current limitations and unfinished features.
- [Development guide](docs/development.md): source layout, building, and testing.

## Contributing

Contributions to the code, documentation, and platform support are welcome.
See the [development guide](docs/development.md) to get started.
