# edut

A command-line framework that lets users define CLI commands, arguments, and behavior in Lua while relying on a C runtime for parsing and execution.

## Installation

The user must have Lua to use the framework, as great part of the Lua configuration is stored and managed using Lua's runtime.

The project can be compiled with CMake using the command:

```
# Builds the project
cmake -B build 

# Compiles with release optimizations
cmake --build build --config Release
```

The executable will be generated in `build/edut`.

You can also install the pre-compiled executable in the repository's releases.

## Collaborating

The framework is simple and certainly need some improvement, so any help for the project will be very well received! It can be with creating a good documentation, adding support to other languages, etc.

