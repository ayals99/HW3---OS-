cmake_minimum_required(VERSION 3.28)
project(HW3___OS_ C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(HW3___OS_
        client.c
        output.c
        Queue.c
        Queue.h
        request.c
        request.h
        segel.c
        segel.h
        server.c)
