#include "std.c"
#include "std/arena.h"


I4 main(I4 argc, char** argv) {
        String foo = String_from_cstr("Hello world!");
        String_print_debug(foo);
        String_print(foo);

        return 0;
}
