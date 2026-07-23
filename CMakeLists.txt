cmake_minimum_required(VERSION 3.15)
project(edut C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Lua REQUIRED)

add_executable( edut
  src/main.c
  src/command.c
  src/util/arena.c
)

target_include_directories( edut PRIVATE
  include/
  ${LUA_INCLUDE_DIR}
)

target_link_libraries( edut PRIVATE
  ${LUA_LIBRARIES}
)

target_compile_options( edut PRIVATE 
  -Wall
  -Wextra
  -g
)

