#include "../std.h"
#include <stdlib.h>


// Private helper function
USize align_up(USize size, USize align);

Arena Arena_new(USize size) {
        return (Arena) {
                .capacity = size,
                .used = 0,
                .buf = (U1*) malloc(size),
        };
}

Void  Arena_free(Arena* arena) {
        arena->capacity = 0;
        arena->used = 0;
        free(arena->buf);
        arena->buf = 0;
}

Ptr Arena_alloc(Arena* arena, USize size, USize align) {
        if (size * align == 0) {
                return NULL;
        }
        if (align  && ((align & (align-1)) == 0)) {
                return NULL;
        }
        
        USize offset = align_up(size, align);
        if (arena->capacity <= offset + size) {
                return NULL;
        }
        arena->used = offset + size;
        return &arena->buf[offset];
}


USize align_up(USize size, USize align) {
        return (size + align - 1) & ~(align-1); 
}

