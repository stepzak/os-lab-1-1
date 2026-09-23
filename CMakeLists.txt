cmake_minimum_required(VERSION 3.30)
project(os_lab_1 C)

set(CMAKE_C_STANDARD 11)

add_executable(os_lab_1 main.c
        common/code/common/auto.h
        common/code/common/vec.h
        common/code/common/vec.c
        common/code/common/unlikely.h
        common/code/os/os.h
        common/code/os/posix/os_posix.h
        common/code/os/posix/os_posix.c)
