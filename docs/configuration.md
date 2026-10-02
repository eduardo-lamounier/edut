# Configuration guide

[README](../README.md) · [Lua API reference](api.md)

Build a small greeting command, add flags, then organize it into subcommands and
modules. You only need basic Lua tables and functions. If you have not built
`edut` yet, follow [installation](../README.md#installation) first; shell examples
here assume `edut` is in your `PATH`.

## On this page

- [Create your configuration](#create-your-configuration)
- [Write your first command](#write-your-first-command)
- [Add flags and help](#add-flags-and-help)
- [Add a subcommand](#add-a-subcommand)
- [Split commands into modules](#split-commands-into-modules)
- [Handle errors](#handle-errors)
- [Troubleshooting](#troubleshooting)

## Create your configuration

On Unix, `edut` loads `init.lua` from `$XDG_CONFIG_HOME/edut` when
`XDG_CONFIG_HOME` is nonempty; otherwise it uses `$HOME/.config/edut`.
It does not fall back to the home directory if the selected file is missing,
and does not automatically load a configuration from your working directory.

```sh
mkdir -p "${XDG_CONFIG_HOME:-$HOME/.config}/edut"
```

Create `init.lua` in that directory. If you already have commands, add the new
command to your existing `commands` list instead of replacing the file.

To experiment separately, you can use a temporary configuration in your current
shell instead:

```sh
export XDG_CONFIG_HOME="$(mktemp -d)"
mkdir -p "$XDG_CONFIG_HOME/edut"
```

The following steps all use the selected directory. Windows support is partial;
see [platform support](known-issues.md#platform-support).

## Write your first command

Put this in `init.lua`:

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

`require "edut"` loads the framework API inside the `edut` executable.
`api.setup` registers a list of commands. Each command has a name in its first
array position and an `execute` function that runs when that command is selected.
You can add more command tables to the same list.

The callback's `input` contains the parsed command-line arguments.
`input.get_argument(1)` returns the first positional argument, or `nil` if absent,
so `or "world"` supplies a default. All argument values are strings, and indices
start at one. Call input functions with a dot (`input.get_argument(1)`), not a
colon (`input:get_argument(1)`).

```sh
edut hello
# Hello, world!
edut hello Ada
# Hello, Ada!
edut hello "Ada Lovelace"
# Hello, Ada Lovelace!
```

The shell's quotes keep a name with spaces in one argument. The framework does
not impose a positional argument count; your callback can validate it.

## Add flags and help

Replace the `hello` command table with this version, keeping it inside the
`commands` list:

```lua
{
  "hello",
  flags = { "--help", "-h", "--upper", ["--greeting"] = 1 },
  execute = function(input)
    if input.contains_flag "--help" or input.contains_flag "-h" then
      print "Usage: edut hello [name] [--greeting text] [--upper]"
      return
    end
    if input.get_argument(2) then error "Expected at most one name" end

    local name = input.get_argument(1) or "world"
    local greeting = "Hello"
    if input.contains_flag "--greeting" then
      greeting = input.get_argument("--greeting", 1)
    end

    local message = greeting .. ", " .. name .. "!"
    if input.contains_flag "--upper" then message = string.upper(message) end
    print(message)
  end,
}
```

A string entry declares a flag without values. A keyed entry such as
`["--greeting"] = 1` declares a flag that consumes one value when supplied.
The flag itself is optional. Check its presence before reading it: requesting
an absent flag's value raises a Lua error.

```sh
edut hello Ada --greeting Hi --upper
# HI, ADA!
edut hello --greeting=Welcome Ada
# Welcome, Ada!
edut hello --help
# Usage: edut hello [name] [--greeting text] [--upper]
```

Command help is written by your callback. The built-in `edut --help` shows only
framework usage; it does not generate help from your command definitions.
Short and long names such as `-h` and `--help` are separate declarations, so the
callback checks both. See [flag definitions](api.md#flag-definitions) for multiple
values and [parsing rules](api.md#command-line-parsing) for exact matching rules.

## Add a subcommand

A parent command groups related actions. Replace your `init.lua` with this
complete example, or add its `example` command to your existing list:

```lua
local api = require "edut"

api.setup {
  commands = {
    {
      "example",
      subcommands = {
        {
          "greet",
          execute = function(input)
            local name = input.get_argument(1) or "world"
            print("Hello, " .. name .. "!")
          end,
        },
      },
      execute = function(input)
        if input.get_argument(1) then error "Unknown example subcommand" end
        local subcommand = input.get_subcommand()
        if subcommand then
          subcommand.execute(input.for_subcommand())
        else
          print "Usage: edut example greet [name]"
        end
      end,
    },
  },
}
```

```sh
edut example
# Usage: edut example greet [name]
edut example greet Ada
# Hello, Ada!
```

Parsing selects `greet`, but only the top-level `example` callback runs
automatically. The parent obtains the selected command with `get_subcommand()`
and calls its `execute` function. `for_subcommand()` supplies the child's input,
so `Ada` is the child's first positional argument. Both accessors return `nil`
when no child was selected.

Each level has its own arguments and flags. Once the parser reaches `greet`,
subsequent tokens belong to `greet`; parent flags must appear before that name.
Children may have their own children, using the same dispatch pattern at each
level. The [bundled example](../config/lua/example.lua) combines subcommands,
flags, validation, and help.

## Split commands into modules

As your configuration grows, move command definitions into `lua/`:

```text
edut/
  init.lua
  lua/
    greetings.lua
```

Put this in `lua/greetings.lua`. A command module returns a list of definitions:

```lua
return {
  {
    "hello",
    execute = function(input)
      print("Hello, " .. (input.get_argument(1) or "world") .. "!")
    end,
  },
}
```

Then register that list in `init.lua`:

```lua
local api = require "edut"
api.setup { commands = require "greetings" }
```

Run `edut hello Ada` as before. The configuration's `lua/?.lua` search path
allows `require "greetings"` to find `lua/greetings.lua`; a name such as
`require "tasks.build"` corresponds to `lua/tasks/build.lua`.

To combine several modules, concatenate their command lists and call `setup`
once. Calling `setup` for each module replaces the previous registration rather
than adding to it. The bundled [init.lua](../config/init.lua) contains a reusable
loader: list your module names in its `load_commands { "example" }` call.

## Handle errors

Use Lua's `error("message")` to stop a callback. An uncaught callback error makes
`edut` exit with failure. Returning `false` from a callback does **not** signal
failure; callback return values are ignored.

A parent can recover from a child's error with `pcall`:

```lua
local subcommand = input.get_subcommand()
if subcommand then
  local ok, message = pcall(subcommand.execute, input.for_subcommand())
  if not ok then
    api.report(tostring(message))
  end
end
```

Here the error is handled, so execution can finish successfully.
[`api.report`](api.md#apireportmessage) only prints a message.
[`api.err`](api.md#apierrmessage) prints a message and immediately exits with
failure, even inside `pcall`. Use it when recovery should not be possible.

## Troubleshooting

| Symptom | What to check |
| --- | --- |
| Cannot load `init.lua` | Check the selected configuration directory and filename. A set `XDG_CONFIG_HOME` takes precedence over `HOME`. |
| `module 'edut' not found` in a standalone Lua interpreter | Run the configuration through `edut`; the executable provides this module. |
| `module 'greetings' not found` | Put the module in the selected configuration's `lua/greetings.lua`. |
| Reading a flag raises an error | Check `contains_flag` first and use the exact declared name, including a trailing `=` if present. |
| A child is selected but nothing happens | The parent must call `subcommand.execute(input.for_subcommand())`. |
| A parent flag fails after a child name | Move it before the child name, or declare and handle a flag on the child. |
| `--help` fails after a command name | Declare and handle command help yourself, as in the flags example. |

For individual function signatures, see the [Lua API reference](api.md).
For remaining platform and configuration limitations, see [known issues](known-issues.md).
