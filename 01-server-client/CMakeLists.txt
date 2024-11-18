# CMake minimal version
cmake_minimum_required(VERSION 3.10)

# Proyect name
project(SocketApp)

# C++ required standar
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Shared files
set(SHARED_SOURCES
  utils.cpp
  utils.h
)

add_executable(client client.cpp utils.cpp)
add_executable(server server.cpp utils.cpp)
