cmake_minimum_required(VERSION 3.30)
project(snake_game)

set(CMAKE_CXX_STANDARD 20)

include_directories("/opt/homebrew/Cellar/sdl3/3.2.0/include")
link_directories("/opt/homebrew/Cellar/sdl3/3.2.0/lib")

add_executable(snake_game main.cpp)

target_link_libraries(snake_game SDL3)
