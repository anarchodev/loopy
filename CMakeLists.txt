cmake_minimum_required(VERSION 3.31)

project(loopy)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -Werror")
set(CMAKE_C_FLAGS_DEBUG "-fsanitize=address -g -O0 -fsanitize=undefined")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(loopy)

include_directories(vendor/libuv/include/ vendor/postgres/src/interfaces/libpq/ vendor/postgres/src/include/)

target_link_libraries(loopy ${CMAKE_SOURCE_DIR}/vendor/libuv/build/libuv.a)

target_sources(loopy
  PRIVATE
  src/main.c
)
