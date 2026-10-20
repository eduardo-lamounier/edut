# Known issues and unfinished features

This document tracks issues identified during the initial codebase review that
remain unresolved. It is a development backlog, not a guarantee that all other
behavior has been validated. Remove or update entries as they are addressed.

## Platform support

The configuration loader recognizes `LOCALAPPDATA` on Windows, but native Windows
builds and runtime behavior still need validation. CMake currently passes
GCC-style compiler options unconditionally.

## Documentation and verification

The [configuration tutorial](configuration.md) introduces command definitions,
input lookup, dispatch, and errors. The [Lua API reference](api.md) documents all
public framework functions, definition tables, and parsing rules.
The CLI regression suite covers core framework behavior and the sample
configuration, including modules loaded from config directories containing Lua
search-path characters on Unix. Native Windows behavior and every Lua API
input/error combination are not exhaustively covered.
