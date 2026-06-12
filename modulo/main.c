#include "nodes.c"

I4 main(I4 argc, char** argv) {
        String code_buf = (String) {.size=0, .capacity=2000, .buf=malloc(2000)};
        I4 c = getchar();
        while (c > 0) {
                String_push(&code_buf, c);
        };
        StringSlice code = String_to_slice(code_buf);
        TokenList toks = TokenList_new(100);
        TokenizerError t_err = tokenize(code, &toks);
        if (t_err) {
                free(code.buf);
                TokenizerError_pretty_print_debug(t_err);
                TokenList_free(&toks);
                return t_err;
        }
        TokenList_pretty_print_debug(toks);
        SynNodeList nodes = SynNodeList_new(100);
        NodeifyResult nr = nodeify_program(TokenSlice_from_list(toks), &nodes);
        if (nr.is_err) {
                free(code.buf);
                NodeifyError_pretty_print_debug(nr.err);
                SynNodeList_pretty_print_debug(nodes);
                TokenList_free(&toks);
                SynNodeList_free(&nodes);
                return nr.err.kind;
        }
        SynNodeList_pretty_print_debug(nodes);
        SynNodeList_pretty_print_tree(nodes);

        // RefineResult rr = refine_program(nr);

        free(code.buf);
        TokenList_free(&toks);
        SynNodeList_free(&nodes);
        return 0;
}
