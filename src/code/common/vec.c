#include "vec.h"

#include <stdlib.h>
#include <string.h>

#include "unlikely.h"

void* v_alloc_with_prefix(size_t prefix_size, size_t item_size, size_t initial_cap) {
    size_t header_size = prefix_size + sizeof(VecHeader);
    size_t total_size = header_size + (item_size * initial_cap);

    void* block = malloc(total_size);
    if (unlikely(!block)) return NULL;

    memset(block, 0, header_size);

    return (char*)block + header_size;
}

VecStatus vector_reserve_impl(void** data, size_t element_size, size_t capacity, size_t prefix) {
    size_t new_size = prefix + sizeof(VecHeader) + capacity * element_size;
    void* old_block = *data ? (char*)v_header(*data) - prefix : NULL;
    size_t header_size = prefix + sizeof(VecHeader);
    void* new_ptr = realloc(old_block, new_size);
    if (unlikely(!new_ptr)) return CVEC_ERROR_BAD_ALLOC;

    VecHeader* h = (VecHeader*)((char*)new_ptr + prefix);
    if (!*data) h->length = 0;
    h->capacity = capacity;
    *data = (char*)new_ptr + header_size;
    return CVEC_SUCCESS;
}

VecStatus vector_grow_impl(void** data, size_t element_size, size_t prefix) {
    size_t new_cap = *data == NULL ? VEC_INIT_CAP : v_header(*data)->capacity * 2;
    return vector_reserve_impl(data, element_size, new_cap, prefix);
}

void vector_remove_impl(void** data, size_t element_size, size_t index) {
    if (unlikely(data == NULL || *data == NULL)) return;
    VecHeader* h = v_header(*data);
    assert(index < h->length && "Index out of range");
    if (index < h->length - 1) {
        char* base = (char*) *data;
        char* dest = base + index * element_size;
        char* src = base + (index + 1) * element_size;

        size_t n = (h->length - index - 1) * element_size;

        memmove((void*) dest, (void*) src, n);
    }
    h->length--;
}

VecStatus vector_shrink_impl(void** data, size_t element_size) {
    if (!data || !*data) return CVEC_SUCCESS;

    size_t len = v_header(*data)->length;
    if (len == 0) {
        free(v_header(*data));
        *data = NULL;
        return CVEC_SUCCESS;
    }

    size_t new_cap = len;
    size_t new_size = sizeof(VecHeader) + (new_cap * element_size);

    void* new_ptr = realloc(v_header(*data), new_size);
    if (unlikely(!new_ptr)) return CVEC_ERROR_BAD_ALLOC;

    VecHeader* h = new_ptr;
    h->capacity = new_cap;

    *data = (void*)(h + 1);
    return CVEC_SUCCESS;
}

VecStatus v_push_raw(void** data, void* el, size_t el_size, size_t prefix) {
    if (*data == NULL || v_header(*data)->length >= v_header(*data)->capacity) {
        VecStatus s = vector_grow_impl(data, el_size, prefix);
        if (unlikely(s != CVEC_SUCCESS)) return s;
    }

    VecHeader* h = v_header(*data);

    char* dest = (char*)(*data) + (el_size * h->length);
    memcpy(dest, el, el_size);
    h->length++;

    return CVEC_SUCCESS;
}