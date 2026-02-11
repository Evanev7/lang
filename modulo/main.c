#include "nodes.c"

I4 main(I4 argc, char** argv) {
        StringSlice code = StringSlice_from_cstr("3U1 ,  -4U81");
        TokenList toks = TokenList_new(100);
        TokenizerError t_err = tokenize(code, &toks);
        if (t_err) {
                TokenizerError_pretty_print_debug(t_err);
                return t_err;
        }
        TokenList_pretty_print_debug(toks);
        SynNodeList nodes = SynNodeList_new(100);
        NodeifyResult nr = nodeify_program(TokenSlice_from_list(toks), &nodes);
        if (nr.is_err) {
                NodeifyError_pretty_print_debug(nr.err);
                return nr.err;
        }
        SynNodeList_pretty_print_debug(nodes);
        return 0;
}
