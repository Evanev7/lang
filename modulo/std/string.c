#include "../std.h"

U1* String_buf_from_cstr(char* str) {
        return (U1*) (unsigned char*) str;
}

char* String_buf_to_cstr(U1* str_buf) {
        return (char*) (unsigned char*) str_buf;
}

String String_from_cstr(char* str) {
        if (str == NULL) {
                return (String) {0};
        }
        USize size = 0;
        while (str[size] != '\0') {
                size += 1;
        }
        return (String) { .size = size, .capacity = size + 1, .buf = String_buf_from_cstr(str) };
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
        return String_buf_to_cstr(str->buf);
}

Void String_print(String str) {
        printf("%s\n", String_buf_to_cstr(str.buf));
}

Void String_print_debug(String str) {
        printf("String(size=%lu,capacity=%lu,buf=%s)\n",
               str.size,
               str.capacity,
               str.buf
        );
}

Void String_extend(String* str_base, String str_add) {
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
