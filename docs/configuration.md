# Configuration guide

[Back to the README](../README.md)

## Defining commands

`edut` loads your `init.lua` before running a command. Call `require "edut"`
to access the API, then pass your commands to `api.setup` as shown in the
[getting started example](../README.md#your-first-command).

Each command uses its first array element as its name and requires an `execute`
function. It can also contain `flags` and `subcommands` tables.

Use dot syntax for input functions, such as `input.get_argument(1)`.
Argument indices start at one. `input.get_argument(index)` reads a positional
argument; `input.get_argument(flag, index)` reads a flag's argument.
`input.contains_flag(name)` checks one flag name. Check that a flag is present
before reading its arguments; looking up an absent flag raises an error.
An out-of-range argument index returns `nil`.

For larger configurations, place modules in the configuration's `lua/` directory
and load them with `require`. The bundled [init.lua](../config/init.lua) shows
how to collect command definitions from modules. Each module returns a list of
commands; see [example.lua](../config/lua/example.lua) for a working example.

## Subcommands

Subcommands use the same definition format as top-level commands. The parent
callback decides whether to execute the selected subcommand:

```lua
execute = function(input)
  local subcommand = input.get_subcommand()
  if subcommand then
    subcommand.execute(input.for_subcommand())
  end
end
```

`get_subcommand()` returns `nil` when no subcommand was selected.
`for_subcommand()` returns its input, and `subcommand.get_name()` returns its
name. Parsing a subcommand does not automatically execute it.

## Flag arguments

Declare a flag without values as a string, such as `flags = {"--verbose"}`.
For a flag with values, use its name as the key and its argument count as the
value, such as `flags = {["--output"] = 1}`.

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

Repeated flags are allowed; lookup returns the first occurrence.

## Errors

Uncaught Lua callback errors produce a nonzero CLI exit. Errors from nested
command callbacks propagate to their caller; Lua can explicitly recover with
`pcall`.

`api.report(message)` prints an error without stopping execution.
`api.err(message)` prints an error and exits with failure; it cannot be caught
with `pcall`. Use Lua's `error` when the caller should be able to handle a failure.

See [known issues](known-issues.md) for configuration and platform limitations.
