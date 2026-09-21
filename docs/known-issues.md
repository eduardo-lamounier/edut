# Known issues and unfinished features

This document tracks issues identified during the initial codebase review that
remain unresolved. It is a development backlog, not a guarantee that all other
behavior has been validated. Remove or update entries as they are addressed.

## Script manager

### Unimplemented subcommands

In `config/lua/scripting.lua`, `scripts add`, `rm`, `list`, `show`, and `edit`
terminate with a “NOT IMPLEMENTED” error. Only `run` has substantive behavior.
Each remaining subcommand needs defined behavior and validation before use.

### Help flag mismatch

The Lua help handler calls `contains_flag("--help", "-h")`, but
`l_parsedinput_containsflag` in `src/command.c` checks only its first argument.
Consequently, `-h` is parsed but does not display help. Either support multiple
names in the API or check each separately in Lua. The help handler also continues
to print “No subcommand passed” after displaying help.

### Shell quoting and script paths

The run handler concatenates script and output paths directly into a shell command.
Paths containing spaces break, and shell metacharacters can change the command
being executed. Script names are not restricted to files inside the scripts
directory. Define the intended path policy and quote shell arguments correctly
before relying on arbitrary names or paths.

### Script results and background execution

Output capture uses `bash script 2>&1 | tee -a output`. The pipeline can report
success because `tee` succeeded even when the script failed. With background
execution, the immediate result describes launching the job, not its completion.
The success message should distinguish these cases and preserve the script's
exit status where execution is synchronous.

### Platform support

Although the C loader now uses `LOCALAPPDATA` on Windows, the Lua script manager
still locates scripts using `XDG_CONFIG_HOME` or `HOME`. It also assumes Bash,
`tput`, `tee`, and Unix redirection. CMake passes GCC-style compiler options
unconditionally. Windows configuration discovery alone does not establish full
Windows support; the script manager and native build need separate validation.

## C runtime and Lua boundary

### Callback errors can return success

`command_execute` in `src/command.c` prints errors from `lua_pcall`, then returns
without propagating a failure status. `main` consequently returns success, and
nested callback failures can also be swallowed. Return or propagate execution
failures through both top-level and subcommand dispatch. Configuration-load
errors now stop execution; this issue concerns errors during command callbacks.

### Command allocation lifetime

Command arrays and recursive subcommand arrays allocated during registration are
not freed. Calling `setup` repeatedly replaces the global command array without
releasing prior arrays or their Lua registry references. The temporary path arena
also leaks on the early configuration-discovery failure path. Define ownership
and cleanup for successful registration, replacement, and partial failure.

### Incomplete public declarations

`include/command.h` declares `pop_lua_parsedinput` and
`parsedinput_containsflag`, but neither has an implementation. Its comment about
checking multiple flags also differs from the actual Lua behavior. Reconcile the
header with the supported API before building on these declarations.

## Documentation and verification

The Lua configuration API still needs a dedicated reference with command schemas,
callback examples, dispatch behavior, and error contracts. Regression tests now
cover configuration fallback, registration limits, and argument parsing on Unix,
but they do not cover script execution, native Windows behavior, or the entire
Lua API. Extend coverage alongside changes to those areas.
