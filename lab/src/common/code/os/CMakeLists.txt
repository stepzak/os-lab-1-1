if(WIN32)
    set(OS_PLATFORM_IMPL win/os_win.c)
else()
    set(OS_PLATFORM_IMPL
            posix/file.c
            posix/pipe.c
            posix/process.c
    )
endif()

add_library(os_lib STATIC
        os.c
        ${OS_PLATFORM_IMPL}
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(os_lib PRIVATE -save-temps=obj -fverbose-asm -g)
endif()

target_include_directories(os_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/os)

target_include_directories(os_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/os/posix)

target_link_libraries(os_lib PUBLIC common_lib)