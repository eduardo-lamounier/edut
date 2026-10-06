# Lua API reference

[README](../README.md) · [Configuration tutorial](configuration.md)

The `edut` executable provides the module loaded by `local api = require "edut"`.
This page covers every public framework function. All functions use **dot
syntax**, including functions on input and command wrappers. Indices are
**one-based**; command-line values are **strings**.

The framework imposes no fixed maximum on the number of commands, subcommands,
registered flags, flag occurrences, positional arguments, or values per flag.
Names and values have no framework-defined length limit. Collections grow as
needed, subject to available resources and operating-system command-line limits.
A value-taking flag still requires exactly the number of values declared in its
definition.

## Function index

| Function | Purpose | Returns |
| --- | --- | --- |
| [`api.setup(options)`](#apisetupoptions) | Register command definitions | No values |
| [`api.report(message)`](#apireportmessage) | Print an error and continue | No values |
| [`api.err(message)`](#apierrmessage) | Print an error and exit with failure | Does not return |
| [`input.get_argument(index)`](#inputget_argumentindex) | Read a positional argument | String or `nil` |
| [`input.get_argument(flag, index)`](#inputget_argumentflag-index) | Read a supplied flag's value | String or `nil`; errors if flag absent |
| [`input.contains_flag(name)`](#inputcontains_flagname) | Check whether a flag was supplied | Boolean |
| [`input.get_subcommand()`](#inputget_subcommand) | Get the selected child command | Command wrapper or `nil` |
| [`input.for_subcommand()`](#inputfor_subcommand) | Get the selected child's arguments | Input wrapper or `nil` |
| [`command.get_name()`](#commandget_name) | Read a command wrapper's name | String |
| [`command.execute(input)`](#commandexecuteinput) | Call a command wrapper's callback | No values |

Also see [command definitions](#command-definitions),
[flag definitions](#flag-definitions), and [command-line parsing](#command-line-parsing).

## Definitions

### Command definitions

Pass command definitions to `api.setup` as a sequential list without holes.
Subcommands use exactly the same definition format.

| Field | Type | Required | Meaning |
| --- | --- | --- | --- |
| `[1]` | String | Yes | Command name, matched exactly on the command line |
| `execute` | Function | Yes | Callback; automatically receives parsed input for a top-level command |
| `flags` | Table | No | Flag definitions; defaults to no flags |
| `subcommands` | List of command tables | No | Child commands; defaults to no children |

```lua
local api = require "edut"
api.setup {
  commands = {
    {
      "hello",
      flags = { "--upper" },
      execute = function(input)
        local name = input.get_argument(1) or "world"
        if input.contains_flag "--upper" then name = string.upper(name) end
        print(name)
      end,
    },
  },
}
```

Only the top-level callback runs automatically. Parents explicitly dispatch to
children. Callback return values are discarded; use Lua `error` or `api.err`
to signal failure. Positional argument counts, defaults, type conversions, and
command help are implemented by your callback, not definition fields.

### Flag definitions

```lua
flags = {
  "--verbose",              -- no values
  "-v",                     -- separate flag, not an automatic alias
  ["--output"] = 1,          -- exactly one value when supplied
  ["--pair"] = 2,            -- exactly two values when supplied
}
```

A list entry declares a flag with no values. A string key with a nonnegative
integer value declares the number of values that flag consumes; zero is also
allowed. Flags are optional unless your callback checks for their presence.
Names are local to each command. Within one command, names must be unique even
after removing a trailing `=`: `--output` and `--output=` cannot both be declared.

Value-taking flags accept separate and attached values:

| Definition | Accepted examples | Name used in Lua lookups |
| --- | --- | --- |
| `["--output"] = 1` | `--output file.txt`, `--output=file.txt` | `"--output"` |
| `["--output-file="] = 1` | `--output-file= file.txt`, `--output-file=file.txt` | `"--output-file="` |
| `["--pair"] = 2` | `--pair first second`, `--pair=first second` | `"--pair"` |

With a name ending in `=`, the exact token `--output-file=` expects a separate
value; use `--output-file= ""` for an empty string. With the name `--output`,
`--output=` supplies an attached empty string.

Repeated flags are accepted. Lookups read the **first** occurrence; there is no
API to enumerate later occurrences. To implement aliases, declare each name and
check each explicitly.

## Module functions

### `api.setup(options)`

Registers the commands in `options.commands`. Returns no values.

- `options`: a table containing `commands`, a sequential list of
  [command definitions](#command-definitions).
- A successful call replaces the previous command list; it does not append.
  `commands = {}` registers an empty list.
- A missing or non-table `commands` field leaves registration unchanged.
- Invalid definitions, such as a missing callback, invalid flag value count, or
  ambiguous flag names, raise a Lua error. A failed registration leaves the
  previously registered commands active if the error is caught.

```lua
local api = require "edut"
api.setup { commands = require "greetings" }
```

Call once from `init.lua` after collecting your commands. See
[modules](configuration.md#split-commands-into-modules) for the file layout.

### `api.report(message)`

Prints an error message and continues. `message` is a string; returns no values.
The current output uses an `ERROR:` prefix and ANSI color on standard output.
This function does not set a failure exit status.

```lua
api.report("Skipping an optional step")
print("Still running")
```

### `api.err(message)`

Prints an error message in the same format as `api.report`, then terminates the
process with failure. `message` is a string. This function never returns and
cannot be caught with `pcall`.

```lua
if not input.get_argument(1) then
  api.err("Expected a filename")
end
```

Use Lua's `error("message")` instead if a caller should be able to recover.
Uncaught Lua errors also cause a nonzero CLI exit.

## Input functions

An input wrapper represents the arguments parsed for **one command level**.
The top-level callback receives one automatically. Use `for_subcommand()` to
obtain the child's wrapper. The following examples run inside an
`execute = function(input) ... end` callback.

### `input.get_argument(index)`

Returns the positional argument at integer `index` as a string, or `nil` if
`index` is below one or past the last argument. Flag values and subcommand names
are not positional arguments. A non-integer index raises a Lua error.

```lua
-- edut hello Ada
local name = input.get_argument(1) or "world"  -- "Ada"
local extra = input.get_argument(2)            -- nil
```

Numbers are not converted automatically: use `tonumber` and validate the result
when your command expects numeric input.

### `input.get_argument(flag, index)`

Returns value `index` from the first occurrence of `flag` as a string, or `nil`
when that index is out of range. `flag` is the exact declared string name;
`index` must be an integer.

**Raises a Lua error if the flag was not supplied**, even if it was declared.
An invalid index type also raises an error. A supplied zero-value flag has no
arguments, so its integer-index lookups return `nil`.

```lua
-- With flags = { ["--output"] = 1 }:
local output = "result.txt"
if input.contains_flag "--output" then
  output = input.get_argument("--output", 1)
end
```

Use a numeric first parameter for positional lookup: `get_argument(1)`.
`get_argument("1", 1)` looks for a flag named `"1"`.

### `input.contains_flag(name)`

Returns `true` if the flag named by string `name` was supplied at this command
level, otherwise `false`. An undeclared name also returns `false`. Names must
match their declarations exactly, including a trailing `=` if present.

```lua
-- With flags = { "--verbose", "-v" }:
local verbose = input.contains_flag "--verbose" or input.contains_flag "-v"
```

This checks the invocation, not merely whether the command declares the flag.

### `input.get_subcommand()`

Returns a command wrapper for the selected immediate child, or `nil` if no child
was selected. The wrapper exposes `get_name()` and `execute(input)`; it is not the
original definition table and does not expose `flags` or `subcommands` fields.
Calling this accessor does not execute anything.

```lua
local child = input.get_subcommand()
if child then print("Selected: " .. child.get_name()) end
```

### `input.for_subcommand()`

Returns the input wrapper for the selected immediate child, or `nil` if no child
was selected. The returned wrapper has the same four input functions documented
above, scoped to that child's arguments and flags.

```lua
local child = input.get_subcommand()
if child then
  child.execute(input.for_subcommand())
end
```

Passing the parent's `input` instead would make the child read the parent's
arguments. Use the child's wrapper for normal dispatch.

## Command wrapper functions

These functions belong to the wrapper returned by `input.get_subcommand()`.
Use a dot, as with input functions.

### `command.get_name()`

Returns the registered command name as a string. Takes no parameters.

```lua
local command = input.get_subcommand()
if command then print(command.get_name()) end
```

### `command.execute(input)`

Calls the command's registered callback with the supplied value. Returns no
values; callback results are discarded. Pass the corresponding child input
wrapper for normal dispatch. The value is forwarded unchanged; omitting it
passes `nil`, which will fail if the callback tries to read input functions.

```lua
local command = input.get_subcommand()
if command then
  command.execute(input.for_subcommand())
end
```

Callback errors propagate to the caller and can be caught with
`pcall(command.execute, input.for_subcommand())`. If uncaught, they cause the CLI
to exit with failure. See [error handling](configuration.md#handle-errors).

## Command-line parsing

- `edut --help` / `-h` and `edut --version` / `-v` work as the first argument
  without loading configuration. After a command name, those tokens have no
  special built-in behavior.
- Command and flag names are case-sensitive. A top-level command must be
  registered. An unrecognized child name becomes a positional argument, so
  validate unexpected arguments in the parent if needed.
- Selecting a subcommand moves parsing into that child. Subsequent flags and
  arguments belong to the child; parent flags are not inherited.
- Flag values must be complete before another recognized flag or child name,
  or any token starting with `--`. An attached first value can contain such a
  name literally: `--output=--verbose`. Quoting a separate value does not bypass
  these rules because the shell removes the quotes before parsing.
- Other tokens become positional arguments, except unrecognized `--...` tokens,
  which cause a parsing error. Undeclared short options such as `-x` become
  positional arguments. There is no special `--` end-of-options marker or
  automatic short-option grouping (`-abc`).
- Missing flag values and other parsing errors stop execution before any command
  callback runs. Configuration loading still happens before argument parsing.

For a walkthrough, return to the [configuration guide](configuration.md).
