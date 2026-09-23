#ifndef OS_H
#define OS_H

#include "../common/vec.h"

typedef int os_file_handle_t;
typedef int os_process_handle_t;

#define OS_INVALID_HANDLE (-1)

typedef enum {
    OS_PROC_FOCUSED = 0,
    OS_PROC_HIDDEN,
    OS_PROC_USE_PARENT_CONSOLE
} os_proc_flags_t;

typedef struct {
    const char *cmd;
    Vec(char*) args;
    const char* workdir;
    os_proc_flags_t flags;
    int execute_as_shell;

    os_file_handle_t redirect_stdin;
    os_file_handle_t redirect_stdout;
    os_file_handle_t redirect_stderr;
} os_process_info_t;

static void os_string_destroy(char **arg) {
    if (!arg) return;
    free(*arg);
    *arg = NULL;
}

VecStatus os_info_add_arg(os_process_info_t *info, const char *arg);

void os_process_info_init(os_process_info_t *info);
void os_process_info_destroy(os_process_info_t *info);

void os_process_info_add_arg(os_process_info_t *info, const char *arg);

os_process_handle_t os_process_create(const os_process_info_t *info);

int os_pipe_create(os_file_handle_t *read_pipe, os_file_handle_t *write_pipe);

int os_pipe_write(os_file_handle_t pipe, const void *buf, int count);
int os_pipe_read(os_file_handle_t pipe, void *buf, int count);

void os_process_close(os_process_handle_t proc);
void os_process_kill(os_process_handle_t proc, unsigned ret_code);

#endif //OS_H
