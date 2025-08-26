#ifndef CPLUS_MOD_STDP
#define CPLUS_MOD_STDP

// ================DEBUG================ //

#ifndef STDP_DEBUG
  #ifndef NDEBUG
    #define STDP_DEBUG 1
  #else
    #define STDP_DEBUG 0
  #endif
#endif

// ============BASIC TYPES============== //

#include <stdint.h>
#include <stdio.h>

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


typedef struct U1List {
        USize size;
        USize capacity;
        U1* buf;
} U1List;

typedef struct String {
        U1List inner;
} String;

typedef enum LogLevel {
        STD_LOG_ERROR = 0,
        STD_LOG_WARN,
        STD_LOG_INFO,
        STD_LOG_DEBUG,
        STD_LOG_TRACE,
} LogLevel;

typedef void* Ptr;

String String_from_cstr(char* str) {
        if (str == NULL) {
                return (String) {0};
        }
        USize size = 0;
        while (str[size] != '\0') {
                size += 1;
        }
        return (String) { .inner = (U1List) { .size= size, .capacity= size + 1, .buf=(U1*)(unsigned char*)str } };
}

void String_print(String str) {
        printf("%s\n", str.inner.buf);
}

void String_print_debug(String str) {
        printf("String(size=%lu,capacity=%lu,buf=%s)\n",
               str.inner.size,
               str.inner.capacity,
               str.inner.buf
        );
}

void String_extend(String str) {
}

Ptr Ptr_with_addr(Ptr ptr, UPtr addr) {
        return ptr = (Ptr) addr;
}
Ptr Ptr_with_offset(Ptr ptr, USize offset) {
        return ptr = (Ptr) ((UPtr) ptr + offset);
}

#endif
