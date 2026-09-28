#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../os/os.h"

#define OS_STDIN_FILENO ((os_file_handle_t)0)
#define READ_BUFFER_SIZE 4096

typedef struct {
    int value;
    int sign;
    bool has_digits;
    bool has_sign;
    int count;
    int sum;
} ParserState;

static void parser_init(ParserState *state) {
    state->value = 0;
    state->sign = 1;
    state->has_digits = false;
    state->has_sign = false;
    state->count = 0;
    state->sum = 0;
}

static void parser_flush(ParserState *state) {
    if (state->has_digits) {
        state->sum += state->value * state->sign;
        state->count++;
    }
    state->value = 0;
    state->sign = 1;
    state->has_digits = false;
    state->has_sign = false;
}


static void parser_process_char(ParserState *state, char ch) {
    if (isspace((unsigned char)ch)) {
        parser_flush(state);
        return;
    }

    if ((ch == '-' || ch == '+') && !state->has_digits && !state->has_sign) {
        if (ch == '-') state->sign = -1;
        state->has_sign = true;
        return;
    }

    if (isdigit((unsigned char)ch)) {
        state->value = state->value * 10 + (ch - '0');
        state->has_digits = true;
        return;
    }

    parser_flush(state);
}

int main(void) {
    char buffer[READ_BUFFER_SIZE];
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    fprintf(stderr, "[CHILD] stdin handle: %p, valid: %d\n",
            hStdin, hStdin != INVALID_HANDLE_VALUE);

    DWORD fileType = GetFileType(hStdin);
    fprintf(stderr, "[CHILD] stdin type: %lu (1=FILE, 2=CHAR, 3=PIPE)\n", fileType);

    ParserState state;
    parser_init(&state);

    os_ssize_t bytes_read;

    while ((bytes_read = os_file_read(OS_STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        for (os_ssize_t i = 0; i < bytes_read; i++) {
            parser_process_char(&state, buffer[i]);
        }
    }

    parser_flush(&state);

    if (bytes_read < 0) {
        perror("Failed to read from stdin");
        return EXIT_FAILURE;
    }

    printf("Sum of %d numbers: %d\n", state.count, state.sum);
    return EXIT_SUCCESS;
}