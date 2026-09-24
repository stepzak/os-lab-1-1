cmake_minimum_required(VERSION 3.30)
project(os_lab_1 C)

set(CMAKE_C_STANDARD 11)

add_executable(os_lab_1 main.c
        common/code/common/auto.h
        common/code/common/vec.h
        common/code/common/vec.c
        common/code/common/unlikely.h
        common/code/os/os.h
        common/code/os/posix/dtors.h
        common/code/os/posix/process.c
        common/code/os/os.c
        common/code/os/posix/pipe.c
        common/code/src/parent.c
        common/code/os/posix/file.c)
