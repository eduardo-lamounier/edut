# Known issues and unfinished features

This document tracks known limitations that remain unresolved. It is not a
guarantee that all other behavior has been validated.

## Platform support

The project has a native Windows CI job that builds with MSVC and runs the CLI
regression suite. Windows behavior outside that suite has not been exhaustively
validated.

## Verification coverage

The [configuration tutorial](configuration.md) introduces command definitions,
input lookup, dispatch, and errors. The [Lua API reference](api.md) documents all
public framework functions, definition tables, and parsing rules. The CLI suite
exercises the public Lua wrappers, the sample configuration, and module loading
from directories containing Lua search-path characters. It does not exhaustively
test every documented input and error combination.
