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

#define Void void

#define I1 int8_t 
#define I2 int16_t 
#define I4 int32_t 
#define I8 int64_t 

#define U1 uint8_t
#define U2 uint16_t 
#define U4 uint32_t
#define U8 uint64_t 

#define UPtr uintptr_t 
#define ISize intptr_t 
#define USize size_t 

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
Void String_extend(String* str_base, const String str_add);
Bool StringSlice_equal(const StringSlice a, const StringSlice b);
Void StringSlice_print(const StringSlice str);
Void StringSlice_print_debug(const StringSlice str);
#define StringSlice_from_cstr(cstr) (StringSlice) { .size=sizeof(cstr)-1, .buf=(U1*)(unsigned char*)cstr }

typedef struct Arena {
        USize capacity;
        USize used;
        U1* buf;
} Arena;

// malloc() a new arena on the heap with size bytes of storage
Arena Arena_new(USize size);
// Free an existing
Void  Arena_free(Arena* arena);
Ptr Arena_alloc(Arena* arena, USize size, USize align);

#define LIST_TRY_PUSH(list, item) if ((list).size < (list).capacity) {(list).buf[(list).size++] = item;}
#define LIST_TRY_PUSH_WITH_EARLY_RETURN(list, item, ret) if ((list).size < (list).capacity) { (list).buf[(list).size++] = item; } else { return ret; }
#define LIST_LAST_UNCHECKED(list) (list).buf[(list).size-1]
#define LIST_NAME_TRY_PUSH(list, name, item) if ((list).size < (list).capacity) {(list).name[(list).size++] = item;}
#define LIST_NAME_TRY_PUSH_WITH_EARLY_RETURN(list, name, item, ret) if ((list).size < (list).capacity) { (list).name[(list).size++] = item; } else { return ret; }
#define LIST_NAME_LAST_UNCHECKED(list, name) (list).name[(list).size-1]

#endif



