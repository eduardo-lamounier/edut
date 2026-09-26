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
`l_parsedinput_containsflag` in `src/lua_api.cpp` checks only its first argument.
Consequently, `-h` is parsed but does not display help. Either support multiple
names in the API or check each separately in Lua. The help handler also continues
to print “No subcommand passed” after displaying help.

### Platform support

Although the C++ loader now uses `LOCALAPPDATA` on Windows, the Lua script manager
still locates scripts using `XDG_CONFIG_HOME` or `HOME`. It also assumes Bash,
`tput`, `tee`, and Unix redirection. CMake passes GCC-style compiler options
unconditionally. Windows configuration discovery alone does not establish full
Windows support; the script manager and native build need separate validation.

## C++ runtime and Lua boundary

### Configuration paths containing Lua search-path separators

The loader appends the configuration directory to `package.path`. A semicolon in
that directory is interpreted as a separator, so configuration modules fail to
load. Literal question marks also conflict with Lua's module-name placeholder.
Supporting these directory names requires a loader that does not encode the
literal directory in a Lua search-path template.

### Command allocation lifetime

Command trees are owned by containers and released at normal process exit.
Calling `setup` repeatedly retains prior trees so command wrappers held by Lua
remain valid. Failed registration leaves the previously published tree unchanged,
including when Lua catches the failure with `pcall`, but partial trees are also
retained until exit. Callback registry references remain until the Lua state is
closed. Earlier reclamation of replaced and failed registrations still needs a
lifetime policy that accounts for retained Lua wrappers. Configuration paths now
use automatic string storage.

## Documentation and verification

The Lua configuration API still needs a dedicated reference with command schemas,
callback examples, dispatch behavior, and error contracts. Regression tests now
cover configuration fallback, registration limits, and argument parsing on Unix,
along with callback failure propagation and disposable script execution tests.
Native Windows behavior and the entire Lua API are not covered. Extend coverage alongside changes to those areas.
