#include "os_posix.h"

#include <stdio.h>

os_process_handle_t os_process_create(const os_process_info_t *info) {
    pid_t pid = fork();

    if (pid < 0) return OS_INVALID_HANDLE;

    if (pid == 0) {
        if (info->redirect_stdin != OS_INVALID_HANDLE) {
            dup2(info->redirect_stdin, STDIN_FILENO);
            close(info->redirect_stdin);
        }
        if (info->redirect_stdout != OS_INVALID_HANDLE) {
            dup2(info->redirect_stdout, STDOUT_FILENO);
            close(info->redirect_stdout);
        }
        if (info->redirect_stderr != OS_INVALID_HANDLE) {
            dup2(info->redirect_stderr, STDERR_FILENO);
            close(info->redirect_stderr);
        }
        char *const *argv = info->args;

        execv(info->cmd, argv);
        perror("os_process_create: execv failed");

        exit(EXIT_FAILURE);
    }
    return pid;
}
