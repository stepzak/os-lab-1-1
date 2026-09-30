#include "../os.h"
#include <windows.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int os_win_error_to_errno(DWORD error) {
    switch (error) {
        case ERROR_SUCCESS:
            return 0;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_DRIVE:
            return ENOENT;
        case ERROR_ACCESS_DENIED:
        case ERROR_SHARING_VIOLATION:
        case ERROR_LOCK_VIOLATION:
        case ERROR_PRIVILEGE_NOT_HELD:
            return EACCES;
        case ERROR_INVALID_HANDLE:
            return EBADF;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            return ENOMEM;
        case ERROR_TOO_MANY_OPEN_FILES:
            return EMFILE;
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:
            return EEXIST;
        case ERROR_DISK_FULL:
        case ERROR_HANDLE_DISK_FULL:
            return ENOSPC;
        case ERROR_BROKEN_PIPE:
        case ERROR_NO_DATA:
        case ERROR_PIPE_NOT_CONNECTED:
            return EPIPE;
        case ERROR_PIPE_BUSY:
            return EAGAIN;
        case ERROR_INVALID_PARAMETER:
            return EINVAL;
        case ERROR_FILENAME_EXCED_RANGE:
            return ENAMETOOLONG;
        default:
            return EIO;
    }
}

static void os_win_set_errno(DWORD error) {
    errno = os_win_error_to_errno(error);
}

static HANDLE os_win_resolve_handle(os_file_handle_t fd) {
    switch (fd) {
        case 0:
            return GetStdHandle(STD_INPUT_HANDLE);
        case 1:
            return GetStdHandle(STD_OUTPUT_HANDLE);
        case 2:
            return GetStdHandle(STD_ERROR_HANDLE);
        default:
            return (HANDLE)fd;
    }
}

static DWORD os_win_io_size(size_t count) {
    return count > MAXDWORD ? MAXDWORD : (DWORD)count;
}

os_file_handle_t os_file_open(const char *path, os_file_flags_t flags) {
    if (!path) {
        errno = EINVAL;
        return OS_INVALID_HANDLE;
    }

    DWORD access = 0;
    if (flags & OS_FILE_READ) access |= GENERIC_READ;
    if (flags & OS_FILE_WRITE) access |= GENERIC_WRITE;
    if (access == 0) access = GENERIC_READ;

    DWORD creation = OPEN_EXISTING;
    if ((flags & OS_FILE_CREATE) && (flags & OS_FILE_TRUNCATE)) {
        creation = CREATE_ALWAYS;
    } else if (flags & OS_FILE_CREATE) {
        creation = OPEN_ALWAYS;
    } else if (flags & OS_FILE_TRUNCATE) {
        creation = TRUNCATE_EXISTING;
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE h = CreateFileA(path, access,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           &sa, creation, FILE_ATTRIBUTE_NORMAL, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        os_win_set_errno(GetLastError());
        return OS_INVALID_HANDLE;
    }

    return (os_file_handle_t)h;
}

void os_file_close(os_file_handle_t fd) {
    if (fd != OS_INVALID_HANDLE) {
        int saved_errno = errno;
        HANDLE handle = os_win_resolve_handle(fd);
        if (handle != NULL && handle != INVALID_HANDLE_VALUE) {
            (void)CloseHandle(handle);
        }
        errno = saved_errno;
    }
}

os_ssize_t os_file_read(os_file_handle_t fd, void *buf, size_t count) {
    if (fd == OS_INVALID_HANDLE || (!buf && count != 0)) {
        errno = EINVAL;
        return -1;
    }
    if (count == 0) return 0;

    HANDLE hFile = os_win_resolve_handle(fd);

    if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return -1;
    }

    DWORD bytes_read = 0;
    if (!ReadFile(hFile, buf, os_win_io_size(count), &bytes_read, NULL)) {
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE || error == ERROR_HANDLE_EOF) {
            return 0;
        }
        os_win_set_errno(error);
        return -1;
    }

    return (os_ssize_t)bytes_read;
}

os_ssize_t os_file_read_full(os_file_handle_t fd, void *buf, size_t count) {
    if (fd == OS_INVALID_HANDLE || (!buf && count != 0)) {
        errno = EINVAL;
        return -1;
    }

    size_t total_read = 0;
    unsigned char *cursor = (unsigned char *)buf;

    while (total_read < count) {
        os_ssize_t bytes_read = os_file_read(
            fd, cursor + total_read, count - total_read);

        if (bytes_read < 0) {
            return total_read > 0 ? (os_ssize_t)total_read : -1;
        }
        if (bytes_read == 0) break;
        total_read += (size_t)bytes_read;
    }

    return (os_ssize_t)total_read;
}

os_ssize_t os_file_write(os_file_handle_t fd, const void *buf, size_t count) {
    if (fd == OS_INVALID_HANDLE || (!buf && count != 0)) {
        errno = EINVAL;
        return -1;
    }
    if (count == 0) return 0;

    HANDLE hFile = os_win_resolve_handle(fd);

    if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return -1;
    }

    DWORD bytes_written = 0;
    if (!WriteFile(hFile, buf, os_win_io_size(count), &bytes_written, NULL)) {
        os_win_set_errno(GetLastError());
        return -1;
    }

    return (os_ssize_t)bytes_written;
}

os_ssize_t os_file_write_full(os_file_handle_t fd, const void *buf, size_t count) {
    if (fd == OS_INVALID_HANDLE || (!buf && count != 0)) {
        errno = EINVAL;
        return -1;
    }

    size_t total_written = 0;
    const unsigned char *cursor = (const unsigned char *)buf;

    while (total_written < count) {
        os_ssize_t bytes_written = os_file_write(
            fd, cursor + total_written, count - total_written);

        if (bytes_written < 0) {
            return total_written > 0 ? (os_ssize_t)total_written : -1;
        }
        if (bytes_written == 0) {
            errno = EIO;
            return total_written > 0 ? (os_ssize_t)total_written : -1;
        }
        total_written += (size_t)bytes_written;
    }

    return (os_ssize_t)total_written;
}

int os_file_fsync(os_file_handle_t fd) {
    if (fd == OS_INVALID_HANDLE) {
        errno = EINVAL;
        return -1;
    }

    HANDLE handle = os_win_resolve_handle(fd);
    if (handle == NULL || handle == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return -1;
    }

    if (!FlushFileBuffers(handle)) {
        os_win_set_errno(GetLastError());
        return -1;
    }
    return 0;
}

int os_pipe_create(os_file_handle_t *read_pipe, os_file_handle_t *write_pipe) {
    if (!read_pipe || !write_pipe) {
        errno = EINVAL;
        return -1;
    }
    *read_pipe = OS_INVALID_HANDLE;
    *write_pipe = OS_INVALID_HANDLE;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE read_handle, write_handle;
    if (!CreatePipe(&read_handle, &write_handle, &sa, 0)) {
        os_win_set_errno(GetLastError());
        return -1;
    }

    *read_pipe = (os_file_handle_t)read_handle;
    *write_pipe = (os_file_handle_t)write_handle;
    return 0;
}

os_ssize_t os_pipe_read(os_file_handle_t pipe, void *buf, int count) {
    if (count < 0) {
        errno = EINVAL;
        return -1;
    }
    return os_file_read(pipe, buf, (size_t)count);
}

os_ssize_t os_pipe_read_full(os_file_handle_t pipe, void *buf, int count) {
    if (count < 0) {
        errno = EINVAL;
        return -1;
    }
    return os_file_read_full(pipe, buf, (size_t)count);
}

os_ssize_t os_pipe_write(os_file_handle_t pipe, const void *buf, int count) {
    if (count < 0) {
        errno = EINVAL;
        return -1;
    }
    return os_file_write(pipe, buf, (size_t)count);
}

os_ssize_t os_pipe_write_full(os_file_handle_t pipe, const void *buf, int count) {
    if (count < 0) {
        errno = EINVAL;
        return -1;
    }
    return os_file_write_full(pipe, buf, (size_t)count);
}

os_process_handle_t os_process_create(const os_process_info_t *info) {
    if (!info || !info->cmd || info->cmd[0] == '\0') {
        errno = EINVAL;
        return OS_INVALID_HANDLE;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = info->redirect_stdin != OS_INVALID_HANDLE 
                   ? (HANDLE)info->redirect_stdin : GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = info->redirect_stdout != OS_INVALID_HANDLE 
                    ? (HANDLE)info->redirect_stdout : GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = info->redirect_stderr != OS_INVALID_HANDLE 
                   ? (HANDLE)info->redirect_stderr : GetStdHandle(STD_ERROR_HANDLE);

    DWORD creation_flags = 0;
    if (info->flags == OS_PROC_HIDDEN) {
        creation_flags |= CREATE_NO_WINDOW;
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }

    char cmdline[4096] = {0};
    size_t offset = 0;
    for (size_t i = 0; i < v_len(info->args); i++) {
        if (info->args[i] == NULL) break;
        int written = snprintf(cmdline + offset, sizeof(cmdline) - offset,
                               "\"%s\" ", info->args[i]);
        if (written < 0 || (size_t)written >= sizeof(cmdline) - offset) {
            errno = E2BIG;
            return OS_INVALID_HANDLE;
        }
        offset += (size_t)written;
    }

    if (!CreateProcessA(
        info->cmd, cmdline, NULL, NULL, TRUE, creation_flags, NULL,
        info->workdir, &si, &pi)) {
        os_win_set_errno(GetLastError());
        return OS_INVALID_HANDLE;
    }

    CloseHandle(pi.hThread);
    return (os_process_handle_t)pi.hProcess;
}

void os_process_close(os_process_handle_t proc) {
    if (proc != OS_INVALID_HANDLE) {
        WaitForSingleObject((HANDLE)proc, INFINITE);
        CloseHandle((HANDLE)proc);
    }
}

void os_process_kill(os_process_handle_t proc, unsigned ret_code) {
    if (proc != OS_INVALID_HANDLE) {
        if (!TerminateProcess((HANDLE)proc, ret_code)) {
            os_win_set_errno(GetLastError());
        }
    }
}
