#include "nodes.c"

I4 main(I4 argc, char** argv) {
        StringSlice code = StringSlice_from_cstr("foo = bar; tree = {x = Fn<>;\n y=x();\n y};");
        TokenList toks = TokenList_new(100);
        TokenizerError t_err = tokenize(code, &toks);
        if (t_err) {
                TokenizerError_pretty_print_debug(t_err);
                TokenList_free(&toks);
                return t_err;
        }
        TokenList_pretty_print_debug(toks);
        SynNodeList nodes = SynNodeList_new(100);
        NodeifyResult nr = nodeify_program(TokenSlice_from_list(toks), &nodes);
        if (nr.is_err) {
                NodeifyError_pretty_print_debug(nr.err);
                SynNodeList_pretty_print_debug(nodes);
                TokenList_free(&toks);
                SynNodeList_free(&nodes);
                return nr.err.kind;
        }
        SynNodeList_pretty_print_debug(nodes);
        SynNodeList_pretty_print_tree(nodes);
        TokenList_free(&toks);
        SynNodeList_free(&nodes);
        return 0;
}
