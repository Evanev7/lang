#include "arena.h"
#include <stdlib.h>

#include "../std.h"

typedef struct Arena {
        USize capacity;
        USize used;
        U1* buf;
} Arena;

// Private helper function
USize align_up(USize size, USize align);

Arena Arena_new(USize size) {
        return (Arena) {
                .capacity = size,
                .used = 0,
                .buf = malloc(size),
        };
}

void  Arena_free(Arena arena) {
        arena.capacity = 0;
        arena.used = 0;
        free(arena.buf);
        arena.buf = 0;
}

Ptr Arena_alloc(Arena arena, USize size, USize align) {
        if (size * align == 0) {
                return NULL;
        }
        if (align  && ((align & (align-1)) == 0)) {
                return NULL;
        }
        
        USize offset = align_up(size, align);
        if (arena.capacity <= offset + size) {
                return NULL;
        }
        arena.used = offset + size;
        return &arena.buf[offset];
}


USize align_up(USize size, USize align) {
        return (size + align - 1) & ~(align-1); 
}

