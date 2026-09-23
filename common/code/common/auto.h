#ifndef AUTO_H
#define AUTO_H
#include "unlikely.h"

#if defined(__clang__) || defined(__GNUC__)

#define DEFINE_CLEANUP_TYPE(TypeName, BaseType, CleanupFunc, InvalidValue) \
    typedef BaseType TypeName;\
    static inline void __cleanup_##TypeName(TypeName *ptr) { \
        if (unlikely(!ptr)) return; \
        if (likely(*ptr != InvalidValue)) { \
            CleanupFunc(*ptr); \
            *ptr = InvalidValue; \
        } \
    } \
    \
    static inline TypeName __##TypeName##_get_invalid_value() { \
        return InvalidValue; \
    } \
    \
    static inline TypeName __##TypeName##_move_out(TypeName *ptr) { \
        if (unlikely(!ptr || *ptr == InvalidValue)) return InvalidValue; \
        \
        TypeName val = *ptr; \
        *ptr = InvalidValue; \
        return val; \
    }

#define AUTO(TypeName) TypeName __attribute__((cleanup(__cleanup_##TypeName)))

#define move_out(TypeName, var) __##TypeName##_move_out(&(var))

#else
#error "This project requires GCC or Clang for RAII support"

#endif

#endif //AUTO_H
