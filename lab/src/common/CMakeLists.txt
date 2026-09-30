add_library(common_lib STATIC code/common/vec.c)

target_include_directories(common_lib PUBLIC code/common)