#include <stdio.h>

#include "../os/os.h"

int main() {
    char filename[256];
    printf("Enter filename: ");
    if (scanf("%255s", filename) != 1) {
        fprintf(stderr, "Failed to read file name from stdin\n");
        return EXIT_FAILURE;
    }

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
    info.cmd = "./build/bin/child";
    os_proc_info_add_arg(&info, "./build/bin/child");

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