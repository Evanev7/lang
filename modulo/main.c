#include "std.h"
#include "tokens.c"

typedef enum SynNodeKind {
        SNK_BLOCK = 0,
        SNK_ASSIGNMENT,
} SynNodeKind;

typedef enum BlockKind {
        BLK_CODE = 0,
        BLK_CCALL,
        BLK_RCALL,
} BlockKind;

typedef struct SynNode {
        struct SynNode* children;
        I4 num_children;
        SynNodeKind kind;
        union {
                BlockKind bk;
        };
} SynNode;

SynNode nodeify(TokenList tokens, Arena arena);

I4 main(I4 argc, char** argv) {
        StringSlice code = StringSlice_from_cstr("hello<with = foo>; loop { big<small>; }");
        TokenList toks = TokenList_new(100);
        TokenizerError err = tokenize(code, &toks);
        if (err) {
                return err;
        }

        TokenList_pretty_print_debug(toks);
        return 0;
}
