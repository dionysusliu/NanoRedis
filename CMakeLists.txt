cmake_minimum_required(VERSION 3.22)
project(my_redis)

set(CMAKE_CXX_STANDARD 14)

include_directories(include/)

add_executable(server
        src/server.cpp
        src/utils.cpp
        src/hashtable.cpp
        src/server_utils.cpp
        src/avl_tree.cpp
        src/zset.cpp
        include/utils.h
        include/hashtable.h
        include/server_utils.h
        include/avl_tree.h
        include/zset.h)

add_executable(client
        src/client.cpp
        src/utils.cpp
        include/utils.h)

