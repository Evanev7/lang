#include "../std.h"
#include <stdio.h>


File* File_open(String path, String mode) {
        return fopen(String_to_cstr(&path), String_to_cstr(&mode));
}
I4 File_close(File* file) {
        return fclose(file);
}
