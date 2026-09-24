#include "dtors.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>


#ifndef CHAR_STACK_SIZE
#define CHAR_STACK_SIZE 1024
#endif


int os_pipe_create(os_file_handle_t *read_pipe, os_file_handle_t *write_pipe) {
    if (!read_pipe || !write_pipe) {
        errno = EINVAL;
        return -1;
    }

    AUTO(AutoFile) fds[2] = { OS_INVALID_HANDLE, OS_INVALID_HANDLE };

    if (unlikely(pipe((int*)fds) < 0)) {
        return -1;
    }

    for (int i = 0; i < 2; i++) {
        if (fds[i] <= STDERR_FILENO) {
            int new_fd = fcntl((int)fds[i], F_DUPFD_CLOEXEC, 3);
            if (new_fd < 0) return -1;
            close((int)fds[i]);
            fds[i] = new_fd;
        }
        int flags = fcntl((int)fds[i], F_GETFD);
        if (flags < 0 || fcntl((int)fds[i], F_SETFD, flags | FD_CLOEXEC) < 0) {
            return -1;
        }
    }

    *read_pipe = move_out(AutoFile, fds[0]);
    *write_pipe = move_out(AutoFile, fds[1]);
    return 0;
}

os_process_handle_t os_process_create(const os_process_info_t *info) {
    (void)info->flags;
    os_file_handle_t r_pipe = OS_INVALID_HANDLE;
    os_file_handle_t w_pipe = OS_INVALID_HANDLE;
    if (unlikely(os_pipe_create(&r_pipe, &w_pipe) < 0)) {
        return OS_INVALID_HANDLE;
    }

    AUTO(AutoFile) parent_read_fd = r_pipe;
    AUTO(AutoFile) child_write_fd = w_pipe;

    pid_t pid = fork();

    if (unlikely(pid < 0)) return OS_INVALID_HANDLE;

    if (pid == 0) {
        close(move_out(AutoFile, parent_read_fd));
        if (info->redirect_stdin != OS_INVALID_HANDLE) {
            if (unlikely(dup2(info->redirect_stdin, STDIN_FILENO) < 0)) {
                int err = errno;
                (void)os_pipe_write_full(child_write_fd, &err, sizeof(err));
                _exit(EXIT_FAILURE);
            }
            close((int)(info->redirect_stdin));
        }
        if (info->redirect_stdout != OS_INVALID_HANDLE) {
            if (unlikely(dup2(info->redirect_stdout, STDOUT_FILENO) < 0)) {
                int err = errno;
                (void)os_pipe_write_full(child_write_fd, &err, sizeof(err));
                _exit(EXIT_FAILURE);
            }
            close((int)info->redirect_stdout);
        }
        if (info->redirect_stderr != OS_INVALID_HANDLE) {
            if (unlikely(dup2(info->redirect_stderr, STDERR_FILENO) < 0)) {
                int err = errno;
                (void)os_pipe_write_full(child_write_fd, &err, sizeof(err));

                _exit(EXIT_FAILURE);
            }
            close((int)info->redirect_stderr);
        }

        if (info->workdir != NULL) {
            if (unlikely(chdir(info->workdir) < 0)) {
                int err = errno;
                (void)os_pipe_write_full(child_write_fd, &err, sizeof(err));
                _exit(EXIT_FAILURE);
            }
        }

        if (info->execute_as_shell) {
            size_t total_len = strlen(info->cmd) + 1;

            for (size_t i = 0; i < v_len(info->args); i++) {
                if (info->args[i] != NULL) {
                    total_len += strlen(info->args[i]) + 1;
                }
            }
            char* shell_cmd;
            char st_buf[CHAR_STACK_SIZE];
            if (total_len <= CHAR_STACK_SIZE) {
                shell_cmd = st_buf;
            } else {
                shell_cmd = malloc(total_len);
                if (unlikely(shell_cmd == NULL)) {
                    int err = ENOMEM;
                    (void)os_pipe_write_full(child_write_fd, &err, sizeof(err));
                    _exit(EXIT_FAILURE);
                }
            }
            size_t offset = 0;
            memcpy(shell_cmd, info->cmd, strlen(info->cmd));
            offset = strlen(info->cmd);
            for (size_t i = 0; i < v_len(info->args); i++) {
                if (info->args[i] == NULL) continue;
                size_t arg_len = strlen(info->args[i]);
                shell_cmd[offset++] = ' ';
                memcpy(shell_cmd + offset, info->args[i], arg_len);
                offset += arg_len;
            }
            shell_cmd[offset] = '\0';
            char* shell_argvs[] = { "/bin/sh", "-c", shell_cmd, NULL };
            execvp("sh", shell_argvs);


            int err = errno;
            (void)os_pipe_write_full(child_write_fd, &err, sizeof(err));
            if (total_len > CHAR_STACK_SIZE) free(shell_cmd);
            _exit(EXIT_FAILURE);
        } else {
            char *const *argv = info->args;

            execv(info->cmd, argv);
            int err = errno;
            (void)os_pipe_write_full(child_write_fd, &err, sizeof(err));

            _exit(EXIT_FAILURE);
        }

        _exit(EXIT_FAILURE);
    }
    close(move_out(AutoFile, child_write_fd));
    int child_errno = 0;
    ssize_t bytes = os_pipe_read_full(parent_read_fd, &child_errno, sizeof(child_errno));

    if (bytes < 0) {
        int status;
        waitpid(pid, &status, 0);
        return OS_INVALID_HANDLE;
    }

    if (bytes > 0) {
        int status;
        waitpid(pid, &status, 0);
        errno = child_errno;
        return OS_INVALID_HANDLE;
    }

    return pid;
}

void os_process_close(os_process_handle_t proc) {
    if (proc != OS_INVALID_HANDLE) {
        int status;
        waitpid((int)proc, &status, 0);
    }
}

void os_process_kill(os_process_handle_t proc, unsigned ret_code) {
    if (proc != OS_INVALID_HANDLE) return;
    (void)ret_code;
    kill((int)proc, SIGKILL);
}