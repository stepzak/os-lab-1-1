#include "../os.h"
#include <windows.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

os_file_handle_t os_file_open(const char *path, os_file_flags_t flags) {
    DWORD access = 0;
    if (flags & OS_FILE_READ) access |= GENERIC_READ;
    if (flags & OS_FILE_WRITE) access |= GENERIC_WRITE;

    DWORD creation = OPEN_EXISTING;
    if ((flags & OS_FILE_CREATE) && (flags & OS_FILE_TRUNCATE)) {
        creation = CREATE_ALWAYS;
    } else if (flags & OS_FILE_CREATE) {
        creation = OPEN_ALWAYS;
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE h = CreateFileA(path, access, FILE_SHARE_READ,
                           &sa, creation, FILE_ATTRIBUTE_NORMAL, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        errno = ENOENT;
        return OS_INVALID_HANDLE;
    }

    return (os_file_handle_t)h;
}

void os_file_close(os_file_handle_t fd) {
    if (fd != OS_INVALID_HANDLE) {
        CloseHandle((HANDLE)fd);
    }
}

os_ssize_t os_file_read(os_file_handle_t fd, void *buf, size_t count) {
    if (fd == OS_INVALID_HANDLE || !buf || count == 0) {
        errno = EINVAL;
        return -1;
    }

    HANDLE hFile;
    if (fd == 0) {
        hFile = GetStdHandle(STD_INPUT_HANDLE);
    } else if (fd == 1) {
        hFile = GetStdHandle(STD_OUTPUT_HANDLE);
    } else if (fd == 2) {
        hFile = GetStdHandle(STD_ERROR_HANDLE);
    } else {
        hFile = (HANDLE)fd;
    }

    if (hFile == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return -1;
    }

    DWORD bytes_read = 0;
    if (!ReadFile(hFile, buf, (DWORD)count, &bytes_read, NULL)) {
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE) {
            return 0;
        }
        errno = EIO;
        return -1;
    }

    return (os_ssize_t)bytes_read;
}

os_ssize_t os_file_write(os_file_handle_t fd, const void *buf, size_t count) {
    if (fd == OS_INVALID_HANDLE || !buf || count == 0) {
        errno = EINVAL;
        return -1;
    }

    HANDLE hFile;
    if (fd == 0) {
        hFile = GetStdHandle(STD_INPUT_HANDLE);
    } else if (fd == 1) {
        hFile = GetStdHandle(STD_OUTPUT_HANDLE);
    } else if (fd == 2) {
        hFile = GetStdHandle(STD_ERROR_HANDLE);
    } else {
        hFile = (HANDLE)fd;
    }

    if (hFile == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return -1;
    }

    DWORD bytes_written = 0;
    if (!WriteFile(hFile, buf, (DWORD)count, &bytes_written, NULL)) {
        errno = EIO;
        return -1;
    }

    return (os_ssize_t)bytes_written;
}

int os_pipe_create(os_file_handle_t *read_pipe, os_file_handle_t *write_pipe) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE read_handle, write_handle;
    if (!CreatePipe(&read_handle, &write_handle, &sa, 0)) {
        return -1;
    }

    *read_pipe = (os_file_handle_t)read_handle;
    *write_pipe = (os_file_handle_t)write_handle;
    return 0;
}

os_ssize_t os_pipe_read(os_file_handle_t pipe, void *buf, int count) {
    if (pipe == OS_INVALID_HANDLE || !buf || count == 0) {
        errno = EINVAL;
        return -1;
    }

    DWORD bytes_read = 0;
    BOOL result = ReadFile((HANDLE)pipe, buf, (DWORD)count, &bytes_read, NULL);
    
    if (!result) {
        DWORD error = GetLastError();
        
        if (error == ERROR_BROKEN_PIPE) {
            return 0; 
        }
        
        if (error == ERROR_NO_DATA) {
            errno = EAGAIN;
            return -1;
        }
        
        errno = EIO;
        return -1;
    }

    return (os_ssize_t)bytes_read;
}

os_ssize_t os_pipe_read_full(os_file_handle_t pipe, void *buf, int count) {
    if (pipe == OS_INVALID_HANDLE || !buf || count == 0) {
        errno = EINVAL;
        return -1;
    }

    size_t total_read = 0;
    char *ptr = (char *)buf;
    size_t remaining = count;

    while (remaining > 0) {
        DWORD bytes_read = 0;
        BOOL result = ReadFile((HANDLE)pipe, ptr, (DWORD)remaining, &bytes_read, NULL);
        
        if (!result) {
            DWORD error = GetLastError();
            if (error == ERROR_BROKEN_PIPE) {
                
                return total_read > 0 ? (os_ssize_t)total_read : 0;
            }
            if (error == ERROR_NO_DATA) {
                errno = EAGAIN;
                return total_read > 0 ? (os_ssize_t)total_read : -1;
            }
            errno = EIO;
            return total_read > 0 ? (os_ssize_t)total_read : -1;
        }

        if (bytes_read == 0) {
            
            break;
        }

        total_read += bytes_read;
        ptr += bytes_read;
        remaining -= bytes_read;
    }

    return (os_ssize_t)total_read;
}

os_ssize_t os_pipe_write(os_file_handle_t pipe, const void *buf, int count) {
    if (pipe == OS_INVALID_HANDLE || !buf || count == 0) {
        errno = EINVAL;
        return -1;
    }

    DWORD bytes_written = 0;
    BOOL result = WriteFile((HANDLE)pipe, buf, (DWORD)count, &bytes_written, NULL);

    if (!result) {
        DWORD error = GetLastError();

        if (error == ERROR_NO_DATA || error == ERROR_BROKEN_PIPE) {
            errno = EPIPE;
            return -1;
        }

        if (error == ERROR_PIPE_BUSY) {
            errno = EAGAIN;
            return -1;
        }
        errno = EIO;
        return -1;
    }

    return (os_ssize_t)bytes_written;
}

os_ssize_t os_pipe_write_full(os_file_handle_t pipe, const void *buf, int count) {
    if (pipe == OS_INVALID_HANDLE || !buf || count == 0) {
        errno = EINVAL;
        return -1;
    }

    size_t total_written = 0;
    const char *ptr = (const char *)buf;
    size_t remaining = count;

    while (remaining > 0) {
        DWORD bytes_written = 0;
        BOOL result = WriteFile((HANDLE)pipe, ptr, (DWORD)remaining, &bytes_written, NULL);
        
        if (!result) {
            DWORD error = GetLastError();
            if (error == ERROR_NO_DATA || error == ERROR_BROKEN_PIPE) {
                errno = EPIPE;
                return total_written > 0 ? (os_ssize_t)total_written : -1;
            }
            if (error == ERROR_PIPE_BUSY) {
                errno = EAGAIN;
                return total_written > 0 ? (os_ssize_t)total_written : -1;
            }
            errno = EIO;
            return total_written > 0 ? (os_ssize_t)total_written : -1;
        }

        if (bytes_written == 0) {
            
            errno = EIO;
            return total_written > 0 ? (os_ssize_t)total_written : -1;
        }

        total_written += bytes_written;
        ptr += bytes_written;
        remaining -= bytes_written;
    }

    return (os_ssize_t)total_written;
}

os_process_handle_t os_process_create(const os_process_info_t *info) {
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

    char cmdline[4096] = {0};
    size_t offset = 0;
    for (size_t i = 0; i < v_len(info->args); i++) {
        if (info->args[i] == NULL) break;
        offset += snprintf(cmdline + offset, sizeof(cmdline) - offset, 
                          "\"%s\" ", info->args[i]);
    }

    if (!CreateProcessA(
        info->cmd, cmdline, NULL, NULL, TRUE, 0, NULL,
        info->workdir, &si, &pi)) {
        DWORD win_err = GetLastError();
        if (win_err == ERROR_FILE_NOT_FOUND || win_err == ERROR_PATH_NOT_FOUND) {
            errno = ENOENT;
        } else if (win_err == ERROR_ACCESS_DENIED) {
            errno = EACCES;
        } else {
            errno = EIO;
        }
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
        TerminateProcess((HANDLE)proc, ret_code);
        CloseHandle((HANDLE)proc);
    }
}

