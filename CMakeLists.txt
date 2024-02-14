cmake_minimum_required(VERSION 3.22)
project(my_redis)

set(CMAKE_CXX_STANDARD 14)

include_directories(include/)

add_executable(server
        include/utils.h
        src/server.cpp
        src/utils.cpp
        src/server_utils.cpp
        include/server_utils.h)

add_executable(client
        include/utils.h
        src/client.cpp
        src/utils.cpp)
