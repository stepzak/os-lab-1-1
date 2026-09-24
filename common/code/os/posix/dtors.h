#ifndef OS_POSIX_H
#define OS_POSIX_H

#include <errno.h>
#include <stdio.h>

#include "../../common/auto.h"
#include "../os.h"
#include <unistd.h>

static void on_file_close(os_file_handle_t *fd) {
    if (unlikely(!fd || *fd == OS_INVALID_HANDLE)) return;
    int saved_errno = errno;

    (void)close(*fd);
    errno = saved_errno;
}

DEFINE_CLEANUP_TYPE(AutoFile, os_file_handle_t, on_file_close, OS_INVALID_HANDLE)

static void string_cleanup(char **char_ptr) {
    if (unlikely(!char_ptr)) return;
    os_string_destroy(char_ptr);
}

DEFINE_CLEANUP_TYPE(AutoString, char*, string_cleanup, NULL)

static void string_vec_cleanup(Vec(char*) *vec_ptr) {
    if (unlikely(!vec_ptr || *vec_ptr == NULL)) return;

    for (size_t i = 0; i < v_len(*vec_ptr); i++) {
        AUTO(AutoString) _s = move_out(AutoString, (*vec_ptr)[i]);
        (void)_s;
    }

    v_free(*vec_ptr);
}

DEFINE_CLEANUP_TYPE(AutoStrVec, Vec(char*), string_vec_cleanup, NULL)

static void proc_info_cleanup(os_process_info_t **info) {
    if (unlikely(!info || *info == NULL)) return;

    os_process_info_destroy(*info);
}

DEFINE_CLEANUP_TYPE(AutoProcInfo, os_process_info_t*, proc_info_cleanup, NULL)

#endif //OS_POSIX_H
