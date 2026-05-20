#include "std.h"
#include "tokens.c"
typedef enum SynNodeKind {
        SNK_PROGRAM,
        SNK_ASSIGNMENT,
        SNK_EXPRESSION,
        SNK_VALUE,
        SNK_BLOCK,
        SNK_RCALL,
        SNK_CCALL,
        SNK_ARGS,
} SynNodeKind;
#define SNK_CASE_DBG_NAME(snk) case snk: return U1_ptr_from_cstr(#snk);
U1* SynNodeKind_retrieve_debug_name(SynNodeKind snk) {
        switch(snk) {
                SNK_CASE_DBG_NAME(SNK_PROGRAM);
                SNK_CASE_DBG_NAME(SNK_ASSIGNMENT);
                SNK_CASE_DBG_NAME(SNK_EXPRESSION);
                SNK_CASE_DBG_NAME(SNK_VALUE);
                SNK_CASE_DBG_NAME(SNK_BLOCK);
                SNK_CASE_DBG_NAME(SNK_RCALL);
                SNK_CASE_DBG_NAME(SNK_CCALL);
                SNK_CASE_DBG_NAME(SNK_ARGS);
        }
        return U1_ptr_from_cstr("SNK_UNKNOWN");
}

typedef enum NodeifyErrorKind {
        NDFERR_NONE,
        NDFERR_NO_MATCH,
        NDFERR_SYNTAX,
        NDFERR_NOT_ENOUGH_NODES,
        NDFERR_NOT_ENOUGH_TOKENS,
} NodeifyErrorKind;
typedef struct NodeifyError {
        NodeifyErrorKind kind;
        USize line;
} NodeifyError;
#define NDFERR_CASE_DBG_NAME(tok) case tok: return U1_ptr_from_cstr(#tok);
U1* NodeifyError_retrieve_debug_name(NodeifyErrorKind n_err) {
        switch(n_err) {
                NDFERR_CASE_DBG_NAME(NDFERR_NONE);
                NDFERR_CASE_DBG_NAME(NDFERR_SYNTAX);
                NDFERR_CASE_DBG_NAME(NDFERR_NO_MATCH);
                NDFERR_CASE_DBG_NAME(NDFERR_NOT_ENOUGH_NODES);
                NDFERR_CASE_DBG_NAME(NDFERR_NOT_ENOUGH_TOKENS);
        };
        return U1_ptr_from_cstr("NDFERR_UNKNOWN");
}
Void NodeifyError_pretty_print_debug(NodeifyError n_err) {
        printf("NodeifyError(%s @ %lu)\n", NodeifyError_retrieve_debug_name(n_err.kind), n_err.line);
}
typedef struct SynNode { SynNodeKind kind; USize child; USize next; USize token_index; } SynNode;
SynNode SynNode_new(SynNodeKind kind, USize index) { return (SynNode) {.kind=kind, .token_index=index, .child=USize_MAX, .next=USize_MAX }; }
typedef struct SynNodeList { SynNode* buf; USize size; USize capacity; } SynNodeList;
SynNodeList SynNodeList_new(USize capacity) {
        return (SynNodeList) {
                .buf=malloc(capacity * sizeof(SynNode)),
                .size=0,
                .capacity=capacity,
        };
}
Void SynNodeList_free(SynNodeList* nodes) {
        free(nodes->buf);
        nodes->size=0;
        nodes->capacity=0;
}
Void SynNodeList_pretty_print_debug(const SynNodeList nodes) {
        printf("SynNodeList(\n    size=%lu,\n    capacity=%lu,\n    nodes=[\n", nodes.size, nodes.capacity);
        for (USize i = 0; i < nodes.size; i+=1) {
                SynNode node = nodes.buf[i];
                printf("        (%4lu): %s,\n", i, U1_ptr_to_cstr(SynNodeKind_retrieve_debug_name(node.kind)));
                if (node.child != USize_MAX && node.next != USize_MAX) {
                        printf("        child: %lu, next: %lu, token_index: %lu\n", node.child, node.next, node.token_index);
                } else if (node.child != USize_MAX) {
                        printf("        child: %lu, next: None, token_index: %lu\n", node.child, node.token_index);
                } else if (node.next != USize_MAX) {
                        printf("        child: None, next: %lu, token_index: %lu\n", node.next, node.token_index);
                } else {
                        printf("        child: None, next: None, token_index: %lu\n", node.token_index);
                }
        }
        printf("])\n");
}
Void SynNodeList_pretty_print_tree_at(const SynNodeList nodes, USize index, USize depth) {
        SynNode node = nodes.buf[index];
        for (USize i=0; i<depth; i++) printf("  ");
        printf("%s\n", U1_ptr_to_cstr(SynNodeKind_retrieve_debug_name(node.kind)));

        if (node.child != USize_MAX) SynNodeList_pretty_print_tree_at(nodes, node.child, depth + 1);
        if (node.next != USize_MAX) SynNodeList_pretty_print_tree_at(nodes, node.next, depth);
}
Void SynNodeList_pretty_print_tree(const SynNodeList nodes) {
        SynNodeList_pretty_print_tree_at(nodes, 0, 0);
}

typedef struct NodeifyResult { 
        Bool is_err;
        union {
                USize ok;
                NodeifyError err;
        };
} NodeifyResult;


NodeifyResult NodeifyResult_ok(USize ok) {
        return (NodeifyResult) { .is_err = false, .ok = ok };
}
#define NDFERR_BAIL(kind) NodeifyResult_err(kind, __LINE__)
NodeifyResult NodeifyResult_err(NodeifyErrorKind kind, USize line) {
        return (NodeifyResult) { .is_err = true, .err=(NodeifyError) { .kind=kind, .line=line } };
}

NodeifyResult NodeifyResult_late(NodeifyResult res) {
        if (res.is_err && res.err.kind == NDFERR_NO_MATCH) {
                res.err.kind = NDFERR_SYNTAX;
        }
        return res; 
}
// Forward decls
NodeifyResult nodeify_assignment(const TokenSlice tokens, USize* index, SynNodeList* nodes);
NodeifyResult nodeify_expression(const TokenSlice tokens, USize* index, SynNodeList* nodes);

#define NODEIFY_SNAPSHOT() USize __start_index=*index; USize __start_nodes=nodes->size
#define NODEIFY_REWIND() *index=__start_index; nodes->size=__start_nodes
#define NODEIFY_FIRST_PEEK(tok) do { if (tokens.size < *index + 1) { return NodeifyResult_err(NDFERR_NOT_ENOUGH_TOKENS, __LINE__); } if (tokens.tok_buf[*index] != tok) { return NodeifyResult_err(NDFERR_NO_MATCH, __LINE__); } } while (0)

NodeifyResult nodeify_arglist(const TokenSlice tokens, USize* index, SynNodeList* nodes, Bool is_const) {
        NODEIFY_SNAPSHOT();
        SynNode head = SynNode_new(SNK_ARGS, *index);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NDFERR_BAIL(NDFERR_NOT_ENOUGH_NODES));
        USize head_idx = nodes->size-1;
        USize prev_idx = USize_MAX;
        while (1) {
                // check for close token
                if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (tokens.tok_buf[*index] == (is_const ? TOK_RANGLE : TOK_RPAREN)) {
                        return NodeifyResult_ok(head_idx);
                }
                // check for expression
                NodeifyResult res = nodeify_expression(tokens, index, nodes);
                if (res.is_err) { NODEIFY_REWIND(); return NodeifyResult_late(res); }
                if (prev_idx == USize_MAX) {
                        nodes->buf[head_idx].child = res.ok;
                } else {
                        nodes->buf[prev_idx].next = res.ok;
                }
                prev_idx = res.ok;
                // check for close token
                if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (tokens.tok_buf[*index] == (is_const ? TOK_RANGLE : TOK_RPAREN)) {
                        return NodeifyResult_ok(head_idx);
                }
                // consume for seperator token
                if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (!Token_is_sep(tokens.tok_buf[*index])) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_SYNTAX); }
                *index += 1;
        }
}
NodeifyResult nodeify_ccall(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
        NODEIFY_FIRST_PEEK(TOK_LANGLE);
        NODEIFY_SNAPSHOT();
        SynNode head = SynNode_new(SNK_CCALL, *index);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NDFERR_BAIL(NDFERR_NOT_ENOUGH_NODES));
        USize head_idx = nodes->size-1;
        *index += 1;
        if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[*index] == TOK_RANGLE) { *index +=1; return NodeifyResult_ok(head_idx); }
        NodeifyResult res = nodeify_arglist(tokens, index, nodes, true);
        if (res.is_err) {
                NODEIFY_REWIND();
                return NodeifyResult_late(res);
        }
        nodes->buf[head_idx].child = res.ok;
        if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[*index] != TOK_RANGLE) { NODEIFY_REWIND(); return NodeifyResult_err(NDFERR_SYNTAX, __LINE__); }
        *index +=1; 
        return NodeifyResult_ok(head_idx);
}
NodeifyResult nodeify_rcall(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
        NODEIFY_FIRST_PEEK(TOK_LPAREN);
        NODEIFY_SNAPSHOT();
        SynNode head = SynNode_new(SNK_RCALL, *index);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NDFERR_BAIL(NDFERR_NOT_ENOUGH_NODES));
        USize head_idx = nodes->size-1;
        *index += 1;
        if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[*index] == TOK_RPAREN) { *index +=1; return NodeifyResult_ok(head_idx); }
        NodeifyResult res = nodeify_arglist(tokens, index, nodes, false);
        if (res.is_err) {
                NODEIFY_REWIND();
                return NodeifyResult_late(res); 
        }
        nodes->buf[head_idx].child = res.ok;
        if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[*index] != TOK_RPAREN) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_SYNTAX); }
        *index +=1; 
        return NodeifyResult_ok(head_idx);
}

/// So currently, we don't handle literals,
/// and we don't handle some kind of magic partial application api
/// we just handle foo<a, b>(c, d)
/// Ideally, foo<x><y> = foo<x, y>, but that's a problem for later.
/// Realistically for now, if you want foo<a><b> = foo<a, b>, define foo2<b> = foo<a, b>
/// Currying is weird man, it just magically stores data in the land of partial application
/// Not a fan.
NodeifyResult nodeify_value(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
        NODEIFY_FIRST_PEEK(TOK_WORD);
        NODEIFY_SNAPSHOT();
        SynNode head = SynNode_new(SNK_VALUE, *index);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NDFERR_BAIL(NDFERR_NOT_ENOUGH_NODES));
        USize head_idx = nodes->size-1;
        *index += 1;
        NodeifyResult res = nodeify_ccall(tokens, index, nodes);
        if (res.is_err) {
                if (res.err.kind != NDFERR_NO_MATCH) {
                        NODEIFY_REWIND();
                        return res;
                }
        } else {
                nodes->buf[head_idx].child = res.ok;
        }
        NodeifyResult res2 = nodeify_rcall(tokens, index, nodes);
        if (res2.is_err) {
                if (res2.err.kind != NDFERR_NO_MATCH) {
                        NODEIFY_REWIND();
                        return res2;
                }
        } else {
                if (res.is_err) {
                        nodes->buf[head_idx].child = res2.ok;
                } else {
                        nodes->buf[res.ok].next = res2.ok;
                }
        }
        return NodeifyResult_ok(head_idx);
}


NodeifyResult nodeify_block(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
        NODEIFY_FIRST_PEEK(TOK_LCURLY);
        NODEIFY_SNAPSHOT();

        SynNode head = SynNode_new(SNK_BLOCK, *index);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NDFERR_BAIL(NDFERR_NOT_ENOUGH_NODES));
        USize head_idx = nodes->size-1;
        *index += 1;

        if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[*index] == TOK_RCURLY) {
                *index += 1;
                return NodeifyResult_ok(head_idx);
        }

        NodeifyResult res = nodeify_expression(tokens, index, nodes);
        if (res.is_err) {
                NODEIFY_REWIND();
                return NodeifyResult_late(res);
        }
        nodes->buf[head_idx].child=res.ok;

        if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[*index] == TOK_RCURLY) {
                *index += 1;
                return NodeifyResult_ok(head_idx);
        }

        while (1) {
                // Got at least one expression, time for (SEP EXPR)* SEP?
                if (tokens.size < *index+1) {
                        NODEIFY_REWIND();
                        return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS);
                }
                if (!Token_is_sep(tokens.tok_buf[*index])) {
                        NODEIFY_REWIND();
                        return NDFERR_BAIL(NDFERR_SYNTAX);
                }

                // We don't push a node for the separator
                *index += 1;

                if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (tokens.tok_buf[*index] == TOK_RCURLY) {
                        *index += 1;
                        return NodeifyResult_ok(head_idx);
                }

                NodeifyResult res2 = nodeify_expression(tokens, index, nodes);
                if (res2.is_err) {
                        NODEIFY_REWIND();
                        return NodeifyResult_late(res2);
                }
                // The previous link points to this, update the previous link, and move on
                nodes->buf[res.ok].next = res2.ok;
                res = res2;

                if (tokens.size < *index+1) { NODEIFY_REWIND(); return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (tokens.tok_buf[*index] == TOK_RCURLY) {
                        *index += 1;
                        return NodeifyResult_ok(head_idx);
                }
        }
}



NodeifyResult nodeify_expression(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
        SynNode head = SynNode_new(SNK_EXPRESSION,*index);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NDFERR_BAIL(NDFERR_NOT_ENOUGH_NODES));
        USize head_idx = nodes->size-1;
        NodeifyResult res;

        res = nodeify_assignment(tokens, index, nodes);
        if (!res.is_err) {
                nodes->buf[head_idx].child = res.ok;
                return NodeifyResult_ok(head_idx);
        }
        if (res.err.kind != NDFERR_NO_MATCH) { nodes->size -= 1; return res; }
        res = nodeify_value(tokens, index, nodes);
        if (!res.is_err) {
                nodes->buf[head_idx].child = res.ok;
                return NodeifyResult_ok(head_idx);
        }
        if (res.err.kind != NDFERR_NO_MATCH) { nodes->size -= 1; return res; }

        res = nodeify_block(tokens, index, nodes);
        if (!res.is_err) {
                nodes->buf[head_idx].child = res.ok;
                return NodeifyResult_ok(head_idx);
        }


        // Failure case, reset nodes and bubble up
        nodes->size -= 1;
        return NodeifyResult_late(res);
}

NodeifyResult nodeify_assignment(const TokenSlice tokens, USize* index, SynNodeList* nodes) {
        // 3 tokens index 0 should succeed, 3 tokens index 1 should fail (WORD=WORD passes, =WORD fails)
        if (tokens.size < *index+2) {
                return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS);
        };
        if (!(tokens.tok_buf[*index] == TOK_WORD && tokens.tok_buf[*index+1] == TOK_EQUAL)) {
                return NDFERR_BAIL(NDFERR_NO_MATCH);
        };
        NODEIFY_SNAPSHOT();
        // Success for the first two tokens, push head into the list and increment index
        SynNode head = SynNode_new(SNK_ASSIGNMENT,*index);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NDFERR_BAIL(NDFERR_NOT_ENOUGH_NODES));
        USize head_idx = nodes->size-1;
        *index += 2;
        // Now nodeify the expression following
        NodeifyResult res = nodeify_expression(tokens, index, nodes);
        if (res.is_err) {
                NODEIFY_REWIND();
                return NodeifyResult_late(res);
        }
        // Connect head to the nodeified expression.
        nodes->buf[head_idx].child = res.ok;

        return NodeifyResult_ok(head_idx);

}

NodeifyResult nodeify_program(const TokenSlice tokens, SynNodeList* nodes) {
        SynNode head = SynNode_new(SNK_PROGRAM, 0);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*nodes, head, NDFERR_BAIL(NDFERR_NOT_ENOUGH_NODES));
        USize index = 0;
        USize head_idx = nodes->size-1;
        USize prev_idx = USize_MAX;
        while (index < tokens.size) {
                NodeifyResult res = nodeify_assignment(tokens, &index, nodes);
                if (res.is_err && res.err.kind == NDFERR_NOT_ENOUGH_TOKENS) {
                        break;
                }
                if (res.is_err) {
                        return res;
                }
                // consume a semicolon after assignment
                if (tokens.size < index+1) { return NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (!Token_is_sep(tokens.tok_buf[index])) {
                        return NDFERR_BAIL(NDFERR_SYNTAX);
                }
                index += 1;
                if (prev_idx == USize_MAX) {
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
* PROGRAM: (ASSIGNMENT SEP)*
* ASSIGNMENT: WORD '=' EXPRESSION SEP
* EXPRESSION: VALUE, BLOCK, ASSIGNMENT
* BLOCK: { (EXPRESSION (SEP EXPRESSION)* SEP?)? }
* VALUE: WORD, WORD CCALL, WORD RCALL, WORD CCALL RCALL
* CCALL: '<'(EXPRESSION (SEP EXPRESSION)*) SEP? (ASSIGNMENT? (SEP ASSIGNMENT)*)'>'
* RCALL: '('(EXPRESSION (SEP EXPRESSION)*) SEP? (ASSIGNMENT? (SEP ASSIGNMENT)*)')'
* SEP: ',' | ';'
*
* If I want {a,b = (1,2), a} rn this will be turned into {a; b = (1,2); a}
* Syntaxless destructuring is nice, but perhaps {(a,b) = (1,2)} is more appropriate?
* Right now, this is still {(a; b) = (1; 2)} - gotta check for bracketing, but I think this is an issue of no literals.
* Should tuples be <>? <3,"foo">? Let's ignore strings for now - they're weird - we only deal in integers
* No, seriously speaking we want a grouping operator, like TypeMul<U1, I8><3U1, -4I8> - very noisy, but acceptable 
* This makes destructuring actually look like TypeMul<U1, I8>(a, b) = my_tup_func() - not sure if I like "value holes"
* but given we will support "type holes" it should be fine
*
* Note, assignments are expressions so I can say everything is an expression - the return value is void
* This simplifies the parser somewhat
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
