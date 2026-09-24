#ifndef OS_H
#define OS_H

#include <stdint.h>

#include "../common/vec.h"

typedef intptr_t os_file_handle_t;
typedef intptr_t os_process_handle_t;
typedef ptrdiff_t os_ssize_t;


#define OS_INVALID_HANDLE (-1)

typedef enum {
    OS_FILE_READ = 1,
    OS_FILE_WRITE = 1 << 1,
    OS_FILE_CREATE = 1 << 2,
    OS_FILE_TRUNCATE = 1 << 3,
} os_file_flags_t;

os_file_handle_t os_file_open(const char *path, os_file_flags_t flags);
void os_file_close(os_file_handle_t file);

os_ssize_t os_file_read(os_file_handle_t file, void *buf, size_t count);
os_ssize_t os_file_read_full(os_file_handle_t file, void *buf, size_t count);


os_ssize_t os_file_write(os_file_handle_t file, const void *buf, size_t count);
os_ssize_t os_file_write_full(os_file_handle_t file, const void *buf, size_t count);

int os_file_fsync(os_file_handle_t file);


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

VecStatus os_proc_info_add_arg(os_process_info_t *info, const char *arg);

void os_process_info_init(os_process_info_t *info);
void os_process_info_destroy(os_process_info_t *info);

os_process_handle_t os_process_create(const os_process_info_t *info);

int os_pipe_create(os_file_handle_t *read_pipe, os_file_handle_t *write_pipe);

os_ssize_t os_pipe_write(os_file_handle_t pipe, const void *buf, int count);
os_ssize_t os_pipe_write_full(os_file_handle_t pipe, const void *buf, int count);

os_ssize_t os_pipe_read(os_file_handle_t pipe, void *buf, int count);
os_ssize_t os_pipe_read_full(os_file_handle_t pipe, void *buf, int count);


void os_process_close(os_process_handle_t proc);
void os_process_kill(os_process_handle_t proc, unsigned ret_code);

#endif //OS_H
