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


ArenaAllocResult Arena_alloc(Arena* arena, USize size, USize align) {
        if (size * align == 0) {
                return (ArenaAllocResult) { .tag = AAERR_ZST };
        }
        if (align  && ((align & (align-1)) == 0)) {
                return (ArenaAllocResult) { .tag = AAERR_NON_POW_2_ALIGN };
        }
        
        USize offset = align_up(size, align);
        if (arena->capacity <= offset + size) {
                return (ArenaAllocResult) { .tag = AAERR_OOM };
        }
        arena->used = offset + size;
        return (ArenaAllocResult) { .tag = AAERR_NONE, .idx = (ArenaHandle) { ._idx = offset } };
}

Ptr Arena_get(const Arena arena, ArenaHandle handle) {
        return &arena.buf[handle._idx];
};


USize align_up(USize size, USize align) {
        return (size + align - 1) & ~(align-1); 
}

