#include <errno.h>
#include <fcntl.h>

#include "../os.h"
#include "dtors.h"

int os_pipe_create(os_file_handle_t *read_pipe, os_file_handle_t *write_pipe) {
    if (!read_pipe || !write_pipe) {
        errno = EINVAL;
        return -1;
    }
    AUTO(AutoFile) r_pipe = OS_INVALID_HANDLE;
    AUTO(AutoFile) w_pipe = OS_INVALID_HANDLE;
    int fds[2];

    if (unlikely(pipe(fds) < 0)) {
        return -1;
    }

    for (int i = 0; i < 2; i++) {
        if (fds[i] <= STDERR_FILENO) {
            int new_fd = fcntl(fds[i], F_DUPFD_CLOEXEC, 3);
            if (new_fd < 0) return -1;
            close(fds[i]);
            fds[i] = new_fd;
        }
        int flags = fcntl((int)fds[i], F_GETFD);
        if (flags < 0 || fcntl((int)fds[i], F_SETFD, flags | FD_CLOEXEC) < 0) {
            return -1;
        }
    }

    r_pipe = fds[0];
    w_pipe = fds[1];

    *read_pipe = move_out(AutoFile, r_pipe);
    *write_pipe = move_out(AutoFile, w_pipe);
    return 0;
}

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