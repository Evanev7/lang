#ifndef STDP
#define STDP

// ================DEBUG================ //

#ifndef STDP_DEBUG
  #ifndef NDEBUG
    #define STDP_DEBUG 1
  #else
    #define STDP_DEBUG 0
  #endif
#endif


#ifndef STDP_DEBUG_ALLOC
  #define STDP_DEBUG_ALLOC STDP_DEBUG
#endif


// ============BASIC TYPES============== //

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#define Void void

#define I1 int8_t 
#define I1_MAX INT8_MAX
#define I1_MIN INT8_MIN
#define I2 int16_t 
#define I2_MAX INT16_MAX
#define I2_MIN INT16_MIN
#define I4 int32_t 
#define I4_MAX INT32_MAX
#define I4_MIN INT32_MIN
#define I8 int64_t 
#define I8_MAX INT64_MAX
#define I8_MIN INT64_MIN

#define U1 uint8_t
#define U1_MAX UINT8_MAX
#define U2 uint16_t 
#define U2_MAX UINT16_MAX
#define U4 uint32_t
#define U4_MAX UINT32_MAX
#define U8 uint64_t 
#define U8_MAX UINT64_MAX

#define UPtr uintptr_t 
#define ISize intptr_t 
#define USize size_t 
#define USize_MAX SIZE_MAX

#define Bool _Bool
#define true 1
#define false 0
#define Void void

typedef void* Ptr;

typedef struct U1List {
        USize size;
        USize capacity;
        U1* buf;
} U1List;

typedef struct U4List {
        USize size;
        USize capacity;
        U4* buf;
} U4List;

typedef FILE File;

typedef struct String {
        USize size;
        USize capacity;
        U1* buf;
} String;

typedef struct StringSlice {
        USize size;
        U1* buf;
} StringSlice;

U1* U1_ptr_from_cstr(char* str);
char* U1_ptr_to_cstr(U1* str_buf);
String String_from_cstr(char* str);
char* String_to_cstr(String* str);
Void String_print(const String str);
Void String_print_debug(const String str);
Void String_push(String* str_base, const char c);
Void String_extend(String* str_base, const String str_add);
StringSlice String_to_slice(const String str);
Bool StringSlice_equal(const StringSlice a, const StringSlice b);
Void StringSlice_print(const StringSlice str);
Void StringSlice_print_debug(const StringSlice str);
#define StringSlice_INIT_CSTR(cstr) { .size=sizeof(cstr)-1, .buf=(U1*)(unsigned char*)cstr }
#define StringSlice_from_cstr(cstr) (StringSlice) StringSlice_INIT_CSTR(cstr)
#define _assert(expr, msg) do { printf("\nAssertion fail:\n" msg "\n"); assert(expr); } while (0)

typedef struct Arena {
        USize capacity;
        USize used;
        U1* buf;
} Arena;

typedef struct ArenaHandle { USize _idx; } ArenaHandle;

typedef struct ArenaAllocResult {
        union {
                ArenaHandle idx;
        };
        enum {
                AAERR_NONE,
                AAERR_ZST,
                AAERR_NON_POW_2_ALIGN,
                AAERR_OOM,
        } tag;
} ArenaAllocResult;

/// malloc() a new arena on the heap with size bytes of storage
Arena Arena_new(USize size);
/// Free an existing arena
Void  Arena_free(Arena* arena);
/// Allocate some space in the arena.
ArenaAllocResult Arena_alloc(Arena* arena, USize size, USize align);
/// Convert a Handle to a Ptr
Ptr Arena_get(const Arena arena, ArenaHandle handle);


#define LIST_TRY_PUSH(list, item) if ((list).size < (list).capacity) { (list).buf[(list).size] = item; (list).size += 1; }
#define LIST_TRY_PUSH_WITH_EARLY_RETURN(list, item, ret) if ((list).size < (list).capacity) { (list).buf[(list).size] = item; (list).size += 1; } else { return ret; }
#define LIST_LAST_UNCHECKED(list) (list).buf[(list).size-1]
#define LIST_NAME_TRY_PUSH(list, name, item) if ((list).size < (list).capacity) { (list).name[(list).size] = item; (list).size += 1;}
#define LIST_NAME_TRY_PUSH_WITH_EARLY_RETURN(list, name, item, ret) if ((list).size < (list).capacity) { (list).name[(list).size] = item; (list).size += 1; } else { return ret; }
#define LIST_NAME_LAST_UNCHECKED(list, name) (list).name[(list).size-1]

#endif



