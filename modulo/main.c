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

// CONVENTION
// If U4List.buf = NULL, U4List.capacity=0 then U4List.size represents the index in the SynNodeList of a SINGLE child.
// This is a stupid niche optimization

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
        const TokenSlice tokens;
        SynNodeList nodes;
        U4 current;
} Nodeifier;

Nodeifier Nodeifier_new(SynNodeList nodes, const TokenSlice tokens) {
        return (Nodeifier) { .idx=0, .tokens=tokens, .nodes=nodes, .current=0 };
}

typedef struct NodeifyResult {
        Bool is_err;
        union {
                struct {
                        // How many tokens we have consumed
                        U4 advance;
                        // What node shall be produced by this step
                        SynNode node;
                };
                NodeifyError err;
        };
} NodeifyResult;

NodeifyResult Nodeify_error(NodeifyError err) {
        return (NodeifyResult) { .is_err=true, .err=err };
}

#define NODEIFY_PUSH(ndf, node) LIST_TRY_PUSH_WITH_EARLY_RETURN(ndf.nodes, node, NDFERR_NODELIST_FULL);

NodeifyResult nodeify_value(Nodeifier* ndf) {

}

NodeifyResult nodeify_expression(Nodeifier* ndf) {

}

NodeifyResult nodeify_assignment(Nodeifier* ndf) {
        // We currently match for WORD EQUAL ANY
        // We do this by checking size >= idx+3 followed by strict equality.
        const TokenSlice tokens = ndf->tokens;
        if (tokens.size < ndf->idx+3) {
                return Nodeify_error(NDFERR_NOT_ENOUGH_TOKENS);
        }
        if (tokens.tok_buf[ndf->idx] == TOK_WORD &&
                tokens.tok_buf[ndf->idx+1] == TOK_EQUAL) {

                SynNode node = (SynNode) { .next=0, .kind=SNK_ASSIGNMENT };
                return (NodeifyResult) { .is_err=false, .advance=4, .node=node };

        }
        return Nodeify_error(NDFERR_NODEIFY_FAILED);
}

NodeifyError nodeify(const TokenSlice tokens, Arena arena, SynNodeList nodes) {
        SynNode head = (SynNode) { .next=0, .kind=SNK_PROGRAM };
        Nodeifier ndf = Nodeifier_new(nodes, tokens);
        NODEIFY_PUSH(ndf, head);
        while (ndf.idx < tokens.size) {
                NodeifyResult res;
                switch (ndf.nodes.buf[ndf.current].kind) {
                        case SNK_PROGRAM: 
                                res = nodeify_assignment(&ndf);
                                break;
                        case SNK_ASSIGNMENT:
                                res = nodeify_expression(&ndf);
                                if (res.is_err) {
                                        res = nodeify_value(&ndf);
                                }
                                break;
                        case SNK_BLOCK:
                        case SNK_RCALL:
                        case SNK_CCALL:
                        default:
                                break;


                }
                if (res.is_err && NodeifyError_is_critical(res.err)) {
                        return res.err;
                } else if (res.is_err) {
                        //continue
                } else {
                        ndf.idx += res.advance;
                        NODEIFY_PUSH(ndf, res.node);
                        ndf.current = ndf.nodes.size - 1;
                        
                }
        }
        return NDFERR_NONE;
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
