cmake_minimum_required(VERSION 3.20)
project(beamformer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(beamformer
    src/main.cpp
    third_party/dr_wav.cpp
)

target_include_directories(beamformer SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party
)

target_compile_options(beamformer PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wshadow
)
