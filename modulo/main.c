#include "std.h"
#include "tokens.c"

typedef enum SynNodeKind {
        SNK_BLOCK = 0,
        SNK_ASSIGNMENT,
        SNK_PROGRAM,
        SNK_CCALL,
        SNK_RCALL,
        SNK_IF,
        SNK_LOOP,
} SynNodeKind;

typedef struct SynNode {
        // SynNodes internally link with a NULL terminated list.
        // This is an Arena index, not a pointer.
        U4 next;
        SynNodeKind kind;
        union {
        };
} SynNode;

typedef enum NodeifyError {
        NDFERR_NONE = 0,
        NDFERR_NOT_ENOUGH_TOKENS,
        NDFERR_NODEIFY_FAILED,
        NDFERR_NODELIST_FULL,
} NodeifyError;

Bool NodeifyError_is_critical(NodeifyError err) {
        return (err == NDFERR_NODELIST_FULL || false);
}

typedef struct SynNodeList {
        SynNode* buf;
        U4 size;
        U4 capacity;
} SynNodeList;

typedef struct Nodeifier {
        USize idx;
        SynNodeList* nodes;
        const TokenSlice tokens;
        U4 current;
} Nodeifier;

Nodeifier Nodeifier_new(SynNodeList* nodes, const TokenSlice tokens) {
        return (Nodeifier) { .idx=0, .tokens=tokens, .nodes=nodes, .current=0 };
}

typedef struct NodeifyResult {
        Bool is_err;
        union {
                Nodeifier ndf;
                NodeifyError err;
        };
} NodeifyResult;

NodeifyResult Nodeify_error(NodeifyError err) {
        return (NodeifyResult) { .is_err=true, .err=err };
}

#define NODEIFY_PUSH(node) LIST_TRY_PUSH_WITH_EARLY_RETURN(*ndf.nodes, node, Nodeify_error(NDFERR_NODELIST_FULL));

NodeifyError nodeify_block(Nodeifier* ndf) {

}

NodeifyError nodeify_value(Nodeifier* ndf) {
        // Matching WORD CCALL, WORD RCALL, WORD CCALL RCALL.
        const TokenSlice tokens = ndf->tokens;
        if (ndf->tokens.size < ndf->idx+2) {
                return NDFERR_NOT_ENOUGH_TOKENS;
        }

}

NodeifyError nodeify_expression(Nodeifier* ndf) {
        if (nodeify_value(ndf)) {
                return nodeify_block(ndf);
        }
        return NDFERR_NONE;
}

NodeifyResult nodeify_assignment(const Nodeifier ndf) {
        // We currently match for WORD EQUAL ANY
        // We do this by checking size >= idx+3 followed by strict equality.
        const TokenSlice tokens = ndf.tokens;
        if (tokens.size < ndf.idx+3) {
                return Nodeify_error(NDFERR_NOT_ENOUGH_TOKENS);
        }
        if (tokens.tok_buf[ndf.idx] != TOK_WORD || 
                tokens.tok_buf[ndf.idx+1] != TOK_EQUAL) {
                return Nodeify_error(NDFERR_NODEIFY_FAILED);
        }

        SynNode* prev = &ndf.nodes->buf[ndf.current];
        SynNode node = (SynNode) { .next=0, .kind=SNK_ASSIGNMENT };
        NODEIFY_PUSH(node);
        prev->next = ndf.nodes->size - 1;
        ndf.idx += 2;
        nodeify_expression(ndf);
}

NodeifyResult nodeify_program(const Nodeifier ndf) {
        while (ndf.idx < ndf.tokens.size) {
                NodeifyResult res = nodeify_assignment(ndf);
                if (res.is_err) { return res; }
        }
        return (NodeifyResult) { .is_err=false, .ndf = ndf };
}

NodeifyError nodeify(const TokenSlice tokens, SynNodeList* nodes) {
        SynNode head = (SynNode) { .next=0, .kind=SNK_PROGRAM };
        LIST_TRY_PUSH(*nodes, head);
        Nodeifier ndf = Nodeifier_new(nodes, tokens);
        return nodeify_program(&ndf);
}


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

/*
*
* Ok, let's write out the grammar.
* PROGRAM: (ASSIGNMENT)*
* ASSIGNMENT: WORD '=' EXPRESSION
* EXPRESSION: VALUE, BLOCK
* BLOCK: { ((EXPRESSION, ASSIGNMENT) SEP)* EXPRESSION }
* VALUE: WORD CCALL, WORD RCALL, WORD CCALL RCALL
* CCALL: '<'(NAME (SEP NAME)*) SEP? (ASSIGNMENT? (SEP ASSIGNMENT)*)'>'
* RCALL: '('NAME* ASSIGNMENT*')'
* SEP: ',', '\n'
*
* Let's write some code and see how it fits.
*
* main = Fn<
*       args = Args<foo = I4
*       bar = I8, baz = Str>
*       ret = I4
*       body = { two = 2U4; four = 4U4; six = two.add(four); six }
* >
*
* Str = struct<
*       size = USize
*       capacity = USize
*       buf = Ptr<U1>
* >
* Ptr = Fn<Type, body = { VoidPtr }>
*
*/
