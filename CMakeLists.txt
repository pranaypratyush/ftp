
cmake_minimum_required(VERSION 2.8)
project(ftp)

set(CMAKE_BUILD_TYPE Debug)
add_executable(ftserv ftserve.c common.c)
add_executable(ftclient ftclient.c common.c)
