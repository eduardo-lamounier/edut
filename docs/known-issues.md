# Known issues and unfinished features

This document tracks issues identified during the initial codebase review that
remain unresolved. It is a development backlog, not a guarantee that all other
behavior has been validated. Remove or update entries as they are addressed.

## Platform support

The configuration loader recognizes `LOCALAPPDATA` on Windows, but native Windows
builds and runtime behavior still need validation. CMake currently passes
GCC-style compiler options unconditionally.

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

The [configuration tutorial](configuration.md) introduces command definitions,
input lookup, dispatch, and errors. The [Lua API reference](api.md) documents all
public framework functions, definition tables, and parsing rules.
The CLI regression suite covers core framework behavior and the sample configuration
on Unix. Native Windows behavior and the entire Lua API are not covered. Extend
coverage alongside changes to those areas.

Some regression tests still assume former fixed limits on positional arguments,
registered flags, repeated flags, and subcommands. These expectations need to be
updated for the current dynamically sized collections.
