# edut

A command-line framework that lets users define CLI commands, arguments, and behavior in Lua while relying on a C runtime for parsing and execution.

## Installation

The user must have Lua to use the framework, as great part of the Lua configuration is stored and managed using Lua's runtime.

You can install the pre-compiled executable in the repository's releases, but if you wish to install from the source:

Firstly clone it:
```
# Clones the project and enters the created directory
git clone https://github.com/eduardo-lamounier/edut
cd edut
```

The project can then be built and compiled with CMake:
```
# Builds the project
cmake -B build 

# Compiles with release optimizations
cmake --build build --config Release
```

The executable will be generated in `build/edut` for you to use it.

You can use the default configs copying its folder to, depending on your OS:

On Linux and MacOS (Unix):
```
mkdir -p ~/.config && cp -r config ~/.config/edut
```

On Windows (Powershell):
```pwsh
cp -Recurse config $env:%LOCALAPPDATA%\edut
```

There you can also make your own changes.

## Collaborating

The framework is simple and certainly needs some improvement, so any help for the project will be very well received! It can be with creating a good documentation, adding support to other languages, adding support for other operating systems etc.

