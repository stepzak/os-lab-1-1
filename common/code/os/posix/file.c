#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <bits/fcntl-linux.h>

#include "../os.h"
#include "../../common/unlikely.h"

os_file_handle_t os_file_open(const char *path, os_file_flags_t flags) {
    if (unlikely(!path)) {
        errno = EINVAL;
        return OS_INVALID_HANDLE;
    }
    int posix_flag = 0;
    if ((flags & OS_FILE_READ) && (flags & OS_FILE_WRITE)) {
        posix_flag |= O_RDWR;
    } else if (flags & OS_FILE_WRITE) {
        posix_flag |= O_WRONLY;
    } else {
        posix_flag |= O_RDONLY;
    }

    if (flags & OS_FILE_CREATE) posix_flag |= O_CREAT;
    if (flags & OS_FILE_TRUNCATE) posix_flag |= O_TRUNC;

    int fd = open(path, posix_flag | O_CLOEXEC, 0644);
    if (fd < 0) {
        return OS_INVALID_HANDLE;
    }
    return fd;
}

void os_file_close(os_file_handle_t fd) {
    if (likely(fd != OS_INVALID_HANDLE)) {
        int e = errno;
        close((int)fd);
        errno = e;
    }
}

os_ssize_t os_file_read(os_file_handle_t file, void *buf, size_t count) {
    if (unlikely(file == OS_INVALID_HANDLE)) {
        errno = EINVAL;
        return -1;
    }
    int fd = (int)file;
    os_ssize_t bytes_read;
    do {
        bytes_read = read(fd, buf, count);
    } while (bytes_read < 0 && errno == EINTR);
    return bytes_read;
}

os_ssize_t os_file_read_full(os_file_handle_t file, void *buf, size_t count) {
    if (unlikely(file == OS_INVALID_HANDLE)) {
        errno = EINVAL;
        return -1;
    }
    int fd = (int)file;
    os_ssize_t total_read = 0;
    size_t remaining = count;
    char* ptr = (char*)buf;
    while (remaining > 0) {
        ssize_t bytes_read = read(fd, ptr, remaining);
        if (bytes_read < 0) {
            if (errno == EINTR) continue;
            return total_read > 0 ? total_read : -1;
        }
        if (bytes_read == 0) break;
        ptr += bytes_read;
        remaining -= bytes_read;
        total_read += bytes_read;
    }
    return total_read;
}

os_ssize_t os_file_write(os_file_handle_t file, const void *buf, size_t count) {
    if (unlikely(file == OS_INVALID_HANDLE)) {
        errno = EINVAL;
        return -1;
    }
    int fd = (int)file;
    os_ssize_t bytes_written;
    do {
        bytes_written = write(fd, buf, count);
    } while (bytes_written < 0 && errno == EINTR);
    return bytes_written;
}

os_ssize_t os_file_write_full(os_file_handle_t file, const void *buf, size_t count) {
    if (unlikely(file == OS_INVALID_HANDLE)) {
        errno = EINVAL;
        return -1;
    }

    int fd = (int)file;
    os_ssize_t total_written = 0;
    size_t remaining = count;
    const char* ptr = buf;
    while (remaining > 0) {
        ssize_t bytes_written = write(fd, ptr, remaining);
        if (bytes_written < 0) {
            if (errno == EINTR) continue;
            return total_written > 0 ? total_written : -1;
        }
        if (bytes_written == 0) {
            errno = EIO;
            return total_written > 0 ? total_written : -1;
        }
        total_written += bytes_written;
        ptr += bytes_written;
        remaining -= bytes_written;
    }
    return total_written;
}

int os_file_fsync(os_file_handle_t file) {
    if (unlikely(file == OS_INVALID_HANDLE)) {
        errno = EINVAL;
        return -1;
    }
    int fd = (int)file;
    int res;
    while ((res = fsync(fd)) < 0 && errno == EINTR) {}
    return res;
}
