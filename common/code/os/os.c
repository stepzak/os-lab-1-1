#include "os.h"
#include <string.h>
#include "../common/unlikely.h"

VecStatus os_proc_info_add_arg(os_process_info_t *info, const char *arg) {
    char *copy = strdup(arg);
    if (unlikely(!copy)) {
        return CVEC_ERROR_BAD_ALLOC;
    }
    size_t orig_len = v_len(info->args);
    int overwrote_null = 0;

    if (orig_len > 0 && info->args[orig_len - 1] == NULL) {
        info->args[orig_len - 1] = copy;
        overwrote_null = 1;
    } else {
        VecStatus status = v_push(info->args, copy);
        if (unlikely(status != CVEC_SUCCESS)) {
            free(copy);
            return status;
        }
    }
    char* nil = NULL;
    VecStatus ret_status = v_push(info->args, nil);
    if (unlikely(ret_status != CVEC_SUCCESS)) {
        if (overwrote_null) {
            info->args[orig_len - 1] = NULL;
        } else {
            v_pop(info->args);
        }

        free(copy);
        return ret_status;
    }

    return CVEC_SUCCESS;
}

void os_process_info_init(os_process_info_t *info) {
    if (!info) return;

    memset(info, 0, sizeof(os_process_info_t));
    info->redirect_stdin  = OS_INVALID_HANDLE;
    info->redirect_stdout = OS_INVALID_HANDLE;
    info->redirect_stderr = OS_INVALID_HANDLE;
}

void os_process_info_destroy(os_process_info_t *info) {
    if (!info || !info->args) return;

    v_free_with_dtor(info->args, os_string_destroy);
    info->args = NULL;
    info->cmd = NULL;
    info->workdir = NULL;
}