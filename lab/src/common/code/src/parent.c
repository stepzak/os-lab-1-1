#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "child_err.h"
#include "../os/os.h"

int main(int argc, char *argv[]) {
    char filename[256];
    char childname[256];

    if (argc != 1 && argc != 3) {
        fprintf(
            stderr,
            "Usage: %s [data_file child_executable]\n",
            argv[0]
        );
        return EXIT_FAILURE;
    }

    if (argc == 1) {
        printf("Enter filename: ");
        fflush(stdout);

        if (fgets(filename, sizeof(filename), stdin) == NULL) {
            fprintf(stderr, "Failed to read file name from stdin\n");
            return EXIT_FAILURE;
        }

        filename[strcspn(filename, "\r\n")] = '\0';

        printf("Enter path to the child process: ");
        fflush(stdout);

        if (fgets(childname, sizeof(childname), stdin) == NULL) {
            fprintf(stderr, "Failed to read child path from stdin\n");
            return EXIT_FAILURE;
        }

        childname[strcspn(childname, "\r\n")] = '\0';
    } else {
        strncpy(filename, argv[1], sizeof(filename) - 1);
        strncpy(childname, argv[2], sizeof(childname) - 1);

        filename[sizeof(filename) - 1] = '\0';
        childname[sizeof(childname) - 1] = '\0';
    }

    os_file_handle_t file =
        os_file_open(filename, OS_FILE_READ);

    if (file == OS_INVALID_HANDLE) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    os_file_handle_t r_pipe = OS_INVALID_HANDLE;
    os_file_handle_t w_pipe = OS_INVALID_HANDLE;

    if (os_pipe_create(&r_pipe, &w_pipe) < 0) {
        perror("Failed to create pipe");
        os_file_close(file);
        return EXIT_FAILURE;
    }

    os_process_info_t info;
    os_process_info_init(&info);
    info.cmd = childname;

    if (os_proc_info_add_arg(&info, childname) != CVEC_SUCCESS) {
        fprintf(
            stderr,
            "Failed to add child process argument, probably out of memory\n"
        );

        os_file_close(file);
        os_file_close(r_pipe);
        os_file_close(w_pipe);
        os_process_info_destroy(&info);

        return EXIT_FAILURE;
    }

    info.redirect_stdin = file;
    info.redirect_stdout = w_pipe;

    os_process_handle_t pid = os_process_create(&info);

    if (pid == OS_INVALID_HANDLE) {
        perror("Failed to create child process");

        os_file_close(file);
        os_file_close(r_pipe);
        os_file_close(w_pipe);
        os_process_info_destroy(&info);

        return EXIT_FAILURE;
    }

    os_file_close(file);
    os_file_close(w_pipe);

    int exit_code = EXIT_SUCCESS;

    for (;;) {
        ChildMessage msg = {0};
        os_ssize_t bytes_read = os_pipe_read_full(
            r_pipe, &msg, sizeof(msg));

        if (bytes_read < 0) {
            perror("Failed to receive child result");
            exit_code = EXIT_FAILURE;
            break;
        }

        if (bytes_read == 0) {
            break;
        }

        if (bytes_read != (os_ssize_t)sizeof(msg)) {
            fprintf(
                stderr,
                "Incomplete child message: expected %zu bytes, received %td\n",
                sizeof(msg),
                bytes_read
            );
            exit_code = EXIT_FAILURE;
            break;
        }

        switch (msg.status) {
        case CHILD_OK:
            printf(
                "Sum of %d numbers: %d\n",
                msg.count,
                msg.result
            );
            fflush(stdout);
            break;

        case CHILD_OVERFLOW:
            fprintf(stderr, "Integer overflow during number parsing\n");
            exit_code = EXIT_FAILURE;
            break;

        case CHILD_SYS_ERR:
            errno = msg.sys_errno;
            perror("Child error");
            exit_code = EXIT_FAILURE;
            break;

        default:
            fprintf(
                stderr,
                "Unknown child status: %d\n",
                (int)msg.status
            );
            exit_code = EXIT_FAILURE;
            break;
        }

        if (exit_code != EXIT_SUCCESS) break;
    }

    os_process_close(pid);
    os_file_close(r_pipe);
    os_process_info_destroy(&info);

    return exit_code;
}
