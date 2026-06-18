#include "nodes.c"

StringSlice t = StringSlice_INIT_CSTR( "\
a = source;                                   \
b = transform(a);                             \
c = transform(a)(b);                          \
d = call(pos, named: value, other: make(arg));\
e = { prepare; finish }(d);                   \
");

I4 main(I4 argc, char** argv) {
        StringSlice code;
        Bool buf_needs_dealloc;
        if (argc==2 && argv[1][0] == '-' && argv[1][1] == '\0') {
                String code_buf = (String) {.size=0, .capacity=2000, .buf=malloc(2000)};
                I4 c = getchar();
                while (c > 0) {
                        String_push(&code_buf, c);
                };
                code = String_to_slice(code_buf);
                buf_needs_dealloc = true;
        } else {
                code = t;
                buf_needs_dealloc = false;
        }
        TokenList toks = TokenList_new(100);
        TokenizerError t_err = tokenize(code, &toks);
        if (t_err) {
                free(code.buf);
                TokenizerError_pretty_print_debug(t_err);
                TokenList_free(&toks);
                return t_err;
        }
        TokenList_pretty_print_debug(toks);
        Parser p = Parser_new(100);
        NodeifyResult nr = nodeify(TokenSlice_from_list(toks), &p);
        if (nr.is_err) {
                if (buf_needs_dealloc)
                        free(code.buf);
                NodeifyError_pretty_print_debug(nr.err);
                Parser_pretty_print_debug(p);
                TokenList_free(&toks);
                Parser_free(&p);
                return nr.err.kind;
        }
        Parser_pretty_print_debug(p);
        Parser_pretty_print_tree(p);

        // RefineResult rr = refine_program(nr);

        if (buf_needs_dealloc)
                free(code.buf);
        TokenList_free(&toks);
        Parser_free(&p);
        return 0;
}
