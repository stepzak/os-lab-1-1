#include <errno.h>
#include <unistd.h>
#include "../../common/unlikely.h"
#include "../os.h"

int os_pipe_write_full(os_file_handle_t pipe, const void *buf, int count) {
    if (unlikely(count < 0 || buf == NULL)) {
        errno = EINVAL;
        return -1;
    }

    size_t remaining = count;
    int total_written = 0;
    char* ptr = (char*) buf;
    while (remaining > 0) {
        int written = (int)write(pipe, ptr, remaining);
        if (unlikely(written < 0)) {
            if (errno == EINTR) continue;
            return total_written > 0 ? total_written : -1;
        }
        if (written == 0) {
            errno = EIO;
            return -1;
        }
        total_written += written;
        ptr += written;
        remaining -= written;
    }
    return total_written;
}

int os_pipe_write(os_file_handle_t pipe, const void *buf, int count) {
    if (unlikely(count < 0 || buf == NULL)) {
        errno = EINVAL;
        return -1;
    }

    int written;
    do { written = (int)write(pipe, buf, count); } while (written < 0 && errno == EINTR);
    return written;
}

int os_pipe_read_full(os_file_handle_t pipe, void *buf, int count) {
    if (unlikely(count < 0 || buf == NULL)) {
        errno = EINVAL;
        return -1;
    }
    size_t remaining = count;
    int total_read = 0;
    char* ptr = (char*) buf;
    while (remaining > 0) {
        int bytes_read = (int)read(pipe, ptr, remaining);
        if (unlikely(bytes_read < 0)) {
            if (errno == EINTR) continue;
            return total_read > 0 ? total_read : -1;
        }
        if (bytes_read == 0) break;
        total_read += bytes_read;
        ptr += bytes_read;
        remaining -= bytes_read;
    }
    return total_read;
}

int os_pipe_read(os_file_handle_t pipe, void *buf, int count) {
    if (unlikely(count < 0 || buf == NULL)) {
        errno = EINVAL;
        return -1;
    }
    int total_read;
    do { total_read = (int)read(pipe, buf, count); } while (total_read < 0 && errno == EINTR);
    return total_read;
}