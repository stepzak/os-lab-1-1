#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../os/os.h"
#include "child_err.h"

#define OS_STDIN_FILENO  ((os_file_handle_t)0)
#define OS_STDOUT_FILENO ((os_file_handle_t)1)
#define READ_BUFFER_SIZE 4096

typedef struct {
    int64_t value;
    int64_t sum;
    int sign;
    bool has_digits;
    bool has_sign;
    int count;
} ParserState;

static void parser_init(ParserState *state) {
    state->value = 0;
    state->sum = 0;
    state->sign = 1;
    state->has_digits = false;
    state->has_sign = false;
    state->count = 0;
}

static void parser_reset_number(ParserState *state) {
    state->value = 0;
    state->sign = 1;
    state->has_digits = false;
    state->has_sign = false;
}

static bool parser_flush(ParserState *state) {
    if (!state->has_digits) {
        parser_reset_number(state);
        return true;
    }

    int64_t number = state->value * state->sign;

    if (number < INT_MIN || number > INT_MAX) {
        return false;
    }

    if ((number > 0 && state->sum > INT_MAX - number) ||
        (number < 0 && state->sum < INT_MIN - number)) {
        return false;
    }

    state->sum += number;
    state->count++;
    parser_reset_number(state);

    return true;
}

static bool parser_process_char(ParserState *state, char ch) {
    if (isspace((unsigned char)ch)) {
        return parser_flush(state);
    }

    if ((ch == '-' || ch == '+') &&
        !state->has_digits &&
        !state->has_sign) {
        state->sign = ch == '-' ? -1 : 1;
        state->has_sign = true;
        return true;
    }

    if (isdigit((unsigned char)ch)) {
        int digit = ch - '0';

        int64_t limit =
            state->sign < 0 ? -(int64_t)INT_MIN : INT_MAX;

        if (state->value > (limit - digit) / 10) {
            return false;
        }

        state->value = state->value * 10 + digit;
        state->has_digits = true;
        return true;
    }

    return parser_flush(state);
}

int main(void) {
    char buffer[READ_BUFFER_SIZE];
    ParserState state;
    ChildMessage msg = {0};

    parser_init(&state);

    os_ssize_t bytes_read = 0;
    bool parsing_failed = false;

    while ((bytes_read = os_file_read(
                OS_STDIN_FILENO,
                buffer,
                sizeof(buffer))) > 0) {

        for (os_ssize_t i = 0; i < bytes_read; i++) {
            if (!parser_process_char(&state, buffer[i])) {
                fprintf(
                    stderr,
                    "Integer overflow while processing number %d\n",
                    state.count + 1
                );

                msg.status = CHILD_OVERFLOW;
                parsing_failed = true;
                break;
            }
        }

        if (parsing_failed) {
            break;
        }
    }

    if (bytes_read < 0) {
        msg.status = CHILD_SYS_ERR;
        msg.sys_errno = errno;
    } else if (msg.status == CHILD_OK && !parser_flush(&state)) {
        msg.status = CHILD_OVERFLOW;
    }

    if (msg.status == CHILD_OK) {
        msg.result = (int)state.sum;
        msg.count = state.count;
    }

    os_ssize_t written = os_file_write_full(
        OS_STDOUT_FILENO,
        &msg,
        sizeof(msg)
    );

    if (written != sizeof(msg)) {
        perror("Failed to send result to parent");
        return EXIT_FAILURE;
    }

    return msg.status == CHILD_OK
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}