add_library(common_lib STATIC code/common/vec.c)

target_include_directories(common_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/common)