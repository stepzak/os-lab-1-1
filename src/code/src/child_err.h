#ifndef CHILD_ERR_H
#define CHILD_ERR_H

typedef enum {
    CHILD_OK = 0,
    CHILD_OVERFLOW,
    CHILD_SYS_ERR
} ChildStatus;

typedef struct {
    ChildStatus status;
    int sys_errno;
    int result;
    int count;
} ChildMessage;

#endif //CHILD_ERR_H
