#include <stdint.h>

#define i8 int8_t 
#define i16 int16_t 
#define i32 int32_t 
#define i64 int64_t 

#define u8 uint8_t
#define u16 uint16_t 
#define u32 uint32_t
#define u64 uint64_t 

#define iptr intptr_t 
#define uptr uintptr_t 
#define isize intptr_t 
#define usize uintptr_t 

#define bool _Bool
#define true 1
#define false 0

#define NULL 0

typedef struct String {
        usize size;
        usize capacity;
        u8* buf;
} String;

String String_from_cstr(char* str) {
        if (str == NULL) {
                return (String) {0};
        }
        usize size = 0;
        while (str[size] != '\0') {
                size += 1
        }
        return (String) { size: size, capacity: size + 1, buf: str }
}

typedef struct FString {
        
}
