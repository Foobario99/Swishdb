cmake_minimum_required(VERSION 3.13)
project(Swishdb)

set(CMAKE_CXX_STANDARD 14)

add_executable(test_create examples/test_create.cpp SwishDB.cpp SwishDB.h)