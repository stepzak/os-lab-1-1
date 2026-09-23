cmake_minimum_required(VERSION 3.30)
project(os_lab_1 C)

set(CMAKE_C_STANDARD 11)

add_executable(os_lab_1 main.c
        common/code/os/posix/auto.h)
