#include "../os.h"

os_ssize_t os_pipe_write_full(os_file_handle_t pipe, const void *buf, int count) {
    return os_file_write_full(pipe, buf, count);
}

os_ssize_t os_pipe_write(os_file_handle_t pipe, const void *buf, int count) {
    return os_file_write(pipe, buf, count);
}

os_ssize_t os_pipe_read_full(os_file_handle_t pipe, void *buf, int count) {
    return os_file_read_full(pipe, buf, count);
}

os_ssize_t os_pipe_read(os_file_handle_t pipe, void *buf, int count) {
    return os_file_read(pipe, buf, count);
}