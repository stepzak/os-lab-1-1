#include <stdio.h>
#include <string.h>

#include "../os/os.h"

int main(int argc, char *argv[]) {
    char filename[256];
    char childname[256];
    if (argc == 2) {
        fprintf(stderr, "Usage: %s <data_file> <child_executable>\n", argv[0]);
        fprintf(stderr, "Example: %s data.txt ./bin/child\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc < 2) {
        printf("Enter filename: ");
        if (fgets(filename, sizeof(filename), stdin) == NULL) {
            fprintf(stderr, "Failed to read file name from stdin\n");
            return EXIT_FAILURE;
        }

        printf("Enter path to the child process: ");
        if (fgets(childname, sizeof(childname), stdin) == NULL) {
            fprintf(stderr, "Failed to read child path from stdin\n");
            return EXIT_FAILURE;
        }
    } else {
        strncpy(filename, argv[1], sizeof(filename) - 1);
        strncpy(childname, argv[2], sizeof(childname) - 1);
    }
    filename[strcspn(filename, "\n")] = '\0';
    childname[strcspn(childname, "\n")] = '\0';

    os_file_handle_t file = os_file_open(filename, OS_FILE_READ);
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
    os_proc_info_add_arg(&info, childname);

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

    char buffer[4096];
    os_ssize_t bytes_read;

    printf("Result from child: ");
    while ((bytes_read = os_pipe_read(r_pipe, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    printf("\n");

    if (bytes_read < 0) {
        perror("Failed to read from pipe");
    }

    os_process_close(pid);
    os_file_close(file);
    os_file_close(r_pipe);
    os_process_info_destroy(&info);

    return EXIT_SUCCESS;
}