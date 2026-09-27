# edut

Define your own command-line commands in Lua and run them through `edut`.
You can add arguments, flags, and subcommands, then choose what each command does.
The bundled configuration includes a Bash script manager.

## Installation

Building from source requires CMake, a compiler supporting C++23, and Lua
development headers and libraries. The bundled script manager also requires Bash;
output capture uses `tee`.

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
`%LOCALAPPDATA%/edut`, but the bundled script manager relies on Unix tools and
configuration paths. See [platform limitations](docs/known-issues.md#platform-support).

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

## Using the bundled script manager

If you prefer to start with the bundled commands, copy the contents of `config/`
from this repository into your configuration directory. For a new installation:

```sh
mkdir -p "${XDG_CONFIG_HOME:-$HOME/.config}/edut"
cp -r config/. "${XDG_CONFIG_HOME:-$HOME/.config}/edut/"
```

This installs the sample `init.lua`, Lua modules, and scripts. If you already
have a configuration, merge the files you want to use instead of replacing it.

Place a readable Bash script, such as `example.sh`, in the configuration's
`scripts/` directory, then run:

```sh
edut scripts run example.sh
edut scripts run example.sh --output-file=run.log
edut scripts run example.sh --on-background --output-file=run.log
```

Scripts run in the directory you invoke `edut` from. Relative output paths also
refer to that directory. Only `scripts run` is implemented; the other bundled
subcommands are unfinished.

See [script execution](docs/configuration.md#script-execution-and-errors) for
filename restrictions, logging, and background behavior.

## Built-in options

- `edut --help` or `edut -h`: show usage.
- `edut --version` or `edut -v`: print the installed version.

These options work without loading your configuration. They apply when passed
as the first argument; flags after a command name are handled by that command.

## More documentation

- [Configuration guide](docs/configuration.md): Lua commands, flags, and script execution.
- [Known issues](docs/known-issues.md): current limitations and unfinished features.
- [Development guide](docs/development.md): source layout, building, and testing.

## Contributing

Contributions to the code, documentation, and platform support are welcome.
See the [development guide](docs/development.md) to get started.
