cmake_minimum_required(VERSION 3.14)
project(PerlParser)

set(CMAKE_CXX_STANDARD 14)

add_executable(PerlParser main.cpp token/Token.h)