#ifndef OS_POSIX_H
#define OS_POSIX_H

#include "../os.h"
#include "../../common/auto.h"
#include <unistd.h>

static void on_file_close(os_file_handle_t *fd) {
    if (unlikely(!fd || *fd == OS_INVALID_HANDLE)) return;

    (void)close(*fd);
    *fd = OS_INVALID_HANDLE;
}

DEFINE_CLEANUP_TYPE(AutoFile, os_file_handle_t, on_file_close, OS_INVALID_HANDLE)

static void string_vec_cleanup(Vec(char*) *vec_ptr) {
    if (unlikely(!vec_ptr || *vec_ptr == NULL)) return;

    for (size_t i = 0; i < v_len(*vec_ptr); i++) {
        free((*vec_ptr)[i]);
    }

    free(*vec_ptr);
}

DEFINE_CLEANUP_TYPE(AutoStrVec, Vec(char*), string_vec_cleanup, NULL)

static void proc_info_cleanup(os_process_info_t **info) {
    if (unlikely(!info || *info == NULL)) return;

    os_process_info_destroy(*info);
    *info = NULL;
}

DEFINE_CLEANUP_TYPE(AutoProcInfo, os_proc_info_t*, proc_info_cleanup, NULL)

#endif //OS_POSIX_H
