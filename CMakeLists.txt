cmake_minimum_required(VERSION 3.20)
project(beamformer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(beamformer
    src/main.cpp
)

target_include_directories(beamformer PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party
)
