cmake_minimum_required(VERSION 3.10)
project(os_lab2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_BUILD_TYPE Release)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_subdirectory(common)
add_subdirectory(common/code/os)

add_executable(parent common/code/src/parent.c)
add_executable(child common/code/src/child.c)

target_link_libraries(parent PRIVATE os_lib)
target_link_libraries(child PRIVATE os_lib)

set_target_properties(parent child PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)