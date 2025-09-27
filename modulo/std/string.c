#include "../std.h"

U1* U1_ptr_from_cstr(char* str) {
        return (U1*) (unsigned char*) str;
}

char* U1_ptr_to_cstr(U1* u1_ptr) {
        return (char*) (unsigned char*) u1_ptr;
}

String String_from_cstr(char* str) {
        if (str == NULL) {
                return (String) { .size=0, .capacity=0, .buf = NULL };
        }
        USize size = 0;
        while (str[size] != '\0') {
                size += 1;
        }
        return (String) { .size = size, .capacity = size + 1, .buf = U1_ptr_from_cstr(str) };
}

char* String_to_cstr(String* str) {
        if (str->buf == NULL) {
                return NULL;
        }
        // If String is full, do not return a valid cstr.
        // As our only constructor currently is String_from_cstr, which guarantees the extra capacity,
        // this failure case shall not occur. A future String_new will need to answer the allocator question.
        if (str->capacity < str->size + 1) {
                return NULL;
        }
        str->buf[str->size+1] = 0;
        return U1_ptr_to_cstr(str->buf);
}

Void String_extend(String* str_base, const String str_add) {
        if (str_base->capacity < str_base->size + str_add.size) {
                return;
        }
        str_base->size += str_add.size;
        // memcpy is for chumps
        for (int i = 0; i < str_add.size; i++) {
                *(str_base->buf + i) = *(str_add.buf + i);
        }
        return;
}

Void String_print(const String str) {
        printf("%s\n", U1_ptr_to_cstr(str.buf));
}

Void String_print_debug(const String str) {
        printf("String(size=%lu,capacity=%lu,buf=%s)\n",
               str.size,
               str.capacity,
               str.buf
        );
}

Void StringSlice_print(const StringSlice str) {
        printf("%s\n", U1_ptr_to_cstr(str.buf));
}

Void StringSlice_print_debug(const StringSlice str) {
        printf("StringSlice(size=%lu,buf=%s)\n",
                str.size,
                str.buf
        );
}

Bool StringSlice_equal(const StringSlice a, const StringSlice b) {
        if (a.size != b.size) {
                return false;
        }
        // memcmp is for chumps
        for (USize i = 0; i < a.size; i+=1) {
                if (a.buf[i] != b.buf[i]) {
                        return false;
                }
        }
        return true;
}


/*
StringSlice StringSlice_from_cstr(char* str) {
        if (str == NULL) {
                return (StringSlice) { .size=0, .buf=NULL };
        }
        USize size = 0;
        while (str[size] != '\0') {
                size += 1;
        }
        return (StringSlice) { .size = size, .buf = U1_ptr_from_cstr(str) };
        
}
*/
