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

U1* String_buf_from_cstr(char* str);
char* String_buf_to_cstr(U1* str_buf);
String String_from_cstr(char* str);
char* String_to_cstr(String* str);
Void String_print(String str);
Void String_print_debug(String str);
Void String_extend(String* str_base, String str_add);

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


#endif



