#include "std.h"
#include "tokens.c"
typedef enum SynNodeKind {
        SNK_PROGRAM,
        SNK_ASSIGNMENT,
        SNK_EXPRESSION,
        SNK_VALUE,
        SNK_BLOCK,
} SynNodeKind;
typedef enum NodeifyError {
        NDFERR_NONE,
        NDFERR_FAILED,
        NDFERR_NOT_ENOUGH_NODES,
        NDFERR_NOT_ENOUGH_TOKENS,
} NodeifyError;
typedef struct SynNode { SynNodeKind kind; USize child; USize next; } SynNode;
typedef struct SynNodeList { SynNode* buf; USize size; USize capacity; } SynNodeList;
typedef struct NodeifyResult { Bool is_err;
        union {
                USize ok;
                NodeifyError err;
        };
} NodeifyResult;

NodeifyResult NodeifyResult_ok(USize ok) {
        return (NodeifyResult) { .is_err = false, .ok = ok };
}
NodeifyResult NodeifyResult_err(NodeifyError err) {
        return (NodeifyResult) { .is_err = true, .err=err };
}

NodeifyResult nodeify_value(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
}

NodeifyResult nodeify_block(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
}

NodeifyResult nodeify_expression(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
        SynNode head = { .kind=SNK_EXPRESSION, .child=0, .next=0 };
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NodeifyResult_err(NDFERR_NOT_ENOUGH_NODES));
        USize head_idx = nodes->size-1;
        NodeifyResult res = nodeify_value(tokens, index, nodes);
        if (res.is_err) {
                res = nodeify_block(tokens, index, nodes);
                if (res.is_err) {
                        nodes->size -= 1;
                        return res;
                }
        }
        return NodeifyResult_ok(head_idx);

}

NodeifyResult nodeify_assignment(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
        // 3 tokens index 0 should succeed, 3 tokens index 1 should fail (WORD=WORD passes, =WORD fails)
        if (tokens.size < *index+2) {
                return NodeifyResult_err(NDFERR_NOT_ENOUGH_TOKENS);
        };
        if (!(tokens.tok_buf[*index] == TOK_WORD && tokens.tok_buf[*index+1] == TOK_EQUAL)) {
                return NodeifyResult_err(NDFERR_FAILED);
        };
        // Success for the first two tokens, push head into the list and increment index
        SynNode head = { .kind=SNK_ASSIGNMENT, .child=0, .next=0 };
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NodeifyResult_err(NDFERR_NOT_ENOUGH_NODES));
        *index += 2;
        USize head_idx = nodes->size-1;
        // Now nodeify the expression following
        NodeifyResult res = nodeify_expression(tokens, index, nodes);
        if (res.is_err) {
                // If it fails, unwind before propagating
                *index -= 2;
                nodes->size -= 1;
                return res;
        }
        // Connect head to the nodeified expression.
        nodes->buf[head_idx].child = res.ok;
        return NodeifyResult_ok(head_idx);

}

NodeifyResult nodeify_program(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
        SynNode head = { .kind=SNK_PROGRAM, .child=0, .next=0 };
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NodeifyResult_err(NDFERR_NOT_ENOUGH_NODES));
        USize head_idx = nodes->size-1;
        USize prev_idx = 0;
        while (true) {
                NodeifyResult res = nodeify_assignment(tokens, index, nodes);
                if (res.is_err) {
                        if (res.err == NDFERR_NOT_ENOUGH_TOKENS || res.err == NDFERR_FAILED) {
                                break;
                        }
                        return res;
                }
                if (prev_idx == 0) {
                        nodes->buf[head_idx].child = res.ok;
                } else {
                        nodes->buf[prev_idx].next = res.ok;
                }
                prev_idx = res.ok;
        }
        return NodeifyResult_ok(head_idx);
}



/*
*
* Ok, let's write out the grammar.
* PROGRAM: (ASSIGNMENT)+
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
