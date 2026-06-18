#include "std.h"
#include "tokens.c"
typedef enum SynNodeKind : U1 {
        SNK_PROGRAM,
        SNK_ASSIGNMENT,
        SNK_KWARG,
        SNK_EXPR,
        SNK_VALUE,
        SNK_RVAR,
        SNK_WVAR,
        SNK_BLOCK,
        SNK_CALL,
} SynNodeKind;
#define SNK_CASE_DBG_NAME(snk) case snk: return U1_ptr_from_cstr(#snk);
U1* SynNodeKind_retrieve_debug_name(SynNodeKind snk) {
        switch(snk) {
                SNK_CASE_DBG_NAME(SNK_PROGRAM);
                SNK_CASE_DBG_NAME(SNK_ASSIGNMENT);
                SNK_CASE_DBG_NAME(SNK_KWARG);
                SNK_CASE_DBG_NAME(SNK_EXPR);
                SNK_CASE_DBG_NAME(SNK_VALUE);
                SNK_CASE_DBG_NAME(SNK_RVAR);
                SNK_CASE_DBG_NAME(SNK_WVAR);
                SNK_CASE_DBG_NAME(SNK_BLOCK);
                SNK_CASE_DBG_NAME(SNK_CALL);
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
        U4 line;
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
        printf("NodeifyError(%s @ %u)\n", U1_ptr_to_cstr(NodeifyError_retrieve_debug_name(n_err.kind)), n_err.line);
}
#define NO_NODE U4_MAX
typedef struct SynNode { 
        U4 first_token; 
        U4 child; 
        U4 next; 
        SynNodeKind kind; 
} SynNode;
SynNode SynNode_new(SynNodeKind kind, U4 index) { return (SynNode) { .kind=kind, .first_token=index, .child=NO_NODE, .next=NO_NODE }; }
typedef struct Parser { SynNode* buf; U4 cursor; U4 size; U4 capacity; } Parser;
Parser Parser_new(U4 capacity) {
        return (Parser) {
                .buf=malloc(capacity * sizeof(SynNode)),
                .cursor=0,
                .size=0,
                .capacity=capacity,
        };
}
Void Parser_free(Parser* p) {
        free(p->buf);
        p->size=0;
        p->capacity=0;
}
/// p, a, b, c: appends c to the tree in p by either making it the child of a (if b is NO_NODE) or the successor of b.
U4 Parser_append_child(Parser* p, U4 head_idx, U4 prev_idx, U4 next_idx) {
        if (prev_idx == NO_NODE) {
                assert(p->buf[head_idx].child == NO_NODE);
                p->buf[head_idx].child = next_idx;
        } else {
                assert(p->buf[prev_idx].next == NO_NODE);
                p->buf[prev_idx].next = next_idx;
        }

        return next_idx;
}
Void Parser_pretty_print_debug(const Parser p) {
        printf("Parser(\n    cursor=%u\n    size=%u,\n    capacity=%u,\n    buf=[\n", p.cursor, p.size, p.capacity);
        for (U4 i = 0; i < p.size; i+=1) {
                SynNode node = p.buf[i];
                printf("        (%4u): %s,\n", i, U1_ptr_to_cstr(SynNodeKind_retrieve_debug_name(node.kind)));
                if (node.child != NO_NODE && node.next != NO_NODE) {
                        printf("        child: %u, next: %u, first_token: %u\n", node.child, node.next, node.first_token);
                } else if (node.child != NO_NODE) {
                        printf("        child: %u, next: None, first_token: %u\n", node.child, node.first_token);
                } else if (node.next != NO_NODE) {
                        printf("        child: None, next: %u, first_token: %u\n", node.next, node.first_token);
                } else {
                        printf("        child: None, next: None, first_token: %u\n", node.first_token);
                }
        }
        printf("])\n");
}
Void Parser_pretty_print_tree_at(const Parser p, U4 index, U2 depth) {
        SynNode node = p.buf[index];
        if (depth == U2_MAX) {
                printf("\nDEPTH LIMIT EXCEEDED\n");
                exit(1);
        }
        for (U4 i=0; i<depth; i++) printf("  ");
        printf("%s\n", U1_ptr_to_cstr(SynNodeKind_retrieve_debug_name(node.kind)));

        if (node.child != NO_NODE) Parser_pretty_print_tree_at(p, node.child, depth + 1);
        if (node.next != NO_NODE) Parser_pretty_print_tree_at(p, node.next, depth);
}
Void Parser_pretty_print_tree(const Parser p) {
        Parser_pretty_print_tree_at(p, 0, 0);
}
typedef struct SynNodeSlice { SynNode* buf; U4 size; } SynNodeSlice;
SynNodeSlice SynNodeSlice_from_list(const Parser p) {
        return (SynNodeSlice) { .buf=p.buf, .size=p.size };
}

typedef struct NodeifyResult { 
        Bool is_err;
        union {
                U4 ok;
                NodeifyError err;
        };
} NodeifyResult;


NodeifyResult NodeifyResult_ok(U4 ok) {
        return (NodeifyResult) { .is_err = false, .ok = ok };
}
NodeifyResult NodeifyResult_err(NodeifyErrorKind kind, U4 line) {
        return (NodeifyResult) { .is_err = true, .err=(NodeifyError) { .kind=kind, .line=line } };
}
#define NDFERR_BAIL(kind) do { *p=snapshot; return NodeifyResult_err(kind, __LINE__); } while (0)

NodeifyResult NodeifyResult_late(NodeifyResult res) {
        if (res.is_err && res.err.kind == NDFERR_NO_MATCH) {
                res.err.kind = NDFERR_SYNTAX;
        }
        return res; 
}

// Forward decls
NodeifyResult nodeify_assignment(const TokenSlice tokens, Parser* p, Bool is_args);
NodeifyResult nodeify_expression(const TokenSlice tokens, Parser* p);

NodeifyResult nodeify_arglist(const TokenSlice tokens, Parser* p) {
        Parser snapshot = *p;
        U4 head_idx = NO_NODE;
        U4 prev_idx = NO_NODE;
        while (1) {
                // check for close token
                if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (tokens.tok_buf[p->cursor] == TOK_RPAREN) { return NodeifyResult_ok(head_idx); }

                // consume expression
                NodeifyResult res = nodeify_assignment(tokens, p, true);
                if (res.is_err) {
                        if (res.err.kind != NDFERR_NO_MATCH) { return NodeifyResult_late(res); }
                        res = nodeify_expression(tokens, p);
                        if (res.is_err) { *p=snapshot; return NodeifyResult_late(res); }
                }

                // res is okay now -- we nodeified a SNK_KWARG at res.ok - thats our "prev" idx, the first child
                // when we're done, we'll have a head node with NO PARENT in the tree, the caller MUST reparent it
                // the remaining KWARGs will follow on from that node
                if (head_idx == NO_NODE) { 
                        head_idx = res.ok; 
                        prev_idx = res.ok;
                } else {
                        assert(p->buf[prev_idx].next == NO_NODE);
                        p->buf[prev_idx].next = res.ok;
                }
                prev_idx = res.ok;

                // check for close token
                if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (tokens.tok_buf[p->cursor] == TOK_RPAREN) { return NodeifyResult_ok(head_idx); }
                // not closed, consume seperator token
                if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (tokens.tok_buf[p->cursor] != TOK_COMMA) { NDFERR_BAIL(NDFERR_SYNTAX); }
                p->cursor += 1;
        }
}

NodeifyResult nodeify_call(const TokenSlice tokens, Parser* p, U4 callee_idx) {
        assert(p->buf[callee_idx].next == NO_NODE);
        Parser snapshot = *p;
        // peek(TOK_LPAREN)
        if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[p->cursor] != TOK_LPAREN) { NDFERR_BAIL(NDFERR_NO_MATCH); }
        // eat(SNK_CALL)
        SynNode head = SynNode_new(SNK_CALL, p->cursor);
        head.child = callee_idx;
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*p, head, NodeifyResult_err(NDFERR_NOT_ENOUGH_NODES, __LINE__));
        p->cursor += 1;

        U4 head_idx = p->size-1;

        // if peek(TOK_RPAREN)
        //     eat(); return ok();
        if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[p->cursor] == TOK_RPAREN) { 
                p->cursor +=1; 
                return NodeifyResult_ok(head_idx); 
        }

        // eat(arglist)
        NodeifyResult res = nodeify_arglist(tokens, p);
        if (res.is_err) { return NodeifyResult_late(res); }
        if (res.ok != NO_NODE) {
                // as part of the contract of nodeify_arglist, we must reparent res.ok onto our call;
                p->buf[callee_idx].next = res.ok;
        }

        // peek(TOK_RPAREN)
        if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[p->cursor] != TOK_RPAREN) { NDFERR_BAIL(NDFERR_SYNTAX); }
        // eat()
        p->cursor +=1; 
        // done!
        return NodeifyResult_ok(head_idx);
}

/// So currently, we don't handle literals, we just handle WORD CALL*
NodeifyResult nodeify_rvar(const TokenSlice tokens, Parser* p) {
        Parser snapshot = *p;
        // peek(TOK_WORD)
        if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[p->cursor] != TOK_WORD) { NDFERR_BAIL(NDFERR_NO_MATCH); }

        // eat(SNK_RVAR)
        SynNode wvar = SynNode_new(SNK_RVAR, p->cursor);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*p, wvar, NodeifyResult_err(NDFERR_NOT_ENOUGH_NODES, __LINE__));
        p->cursor += 1;

        U4 head_idx = p->size - 1;
        return NodeifyResult_ok(head_idx);
}

NodeifyResult nodeify_block(const TokenSlice tokens, Parser* p) {
        Parser snapshot = *p;

        // peek(TOK_LCURLY)
        if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[p->cursor] != TOK_LCURLY) { NDFERR_BAIL(NDFERR_NO_MATCH); }
        // eat(SNK_BLOCK)
        SynNode head = SynNode_new(SNK_BLOCK, p->cursor);
        p->cursor += 1;
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*p, head, NodeifyResult_err(NDFERR_NOT_ENOUGH_NODES, __LINE__));

        U4 head_idx = p->size-1;
        U4 prev_idx = NO_NODE;
        while (1) {
                // if peek(TOK_RCURLY)
                //     eat(); return ok();
                if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (tokens.tok_buf[p->cursor] == TOK_RCURLY) {
                        p->cursor += 1;
                        return NodeifyResult_ok(head_idx);
                }

                NodeifyResult res = nodeify_expression(tokens, p);
                if (res.is_err) {
                        *p = snapshot;
                        return NodeifyResult_late(res);
                }
                // The previous link points to this, update the previous link, and move on
                prev_idx = Parser_append_child(p, head_idx, prev_idx, res.ok);

                // if !peek(TOK_SEMICOLON)
                //     require(TOK_RCURLY)
                if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                Token at = tokens.tok_buf[p->cursor];
                if (at == TOK_SEMICOLON) { 
                        // don't push a node for the separator
                        p->cursor += 1;
                        continue;
                }
                if (at == TOK_RCURLY) {
                        p->cursor += 1;
                        return NodeifyResult_ok(head_idx);
                }
                
                NDFERR_BAIL(NDFERR_SYNTAX); 
        }
}

NodeifyResult nodeify_expression(const TokenSlice tokens, Parser* p) {
        Parser snapshot = *p;
        U4 head_idx = NO_NODE;
        NodeifyResult res;
        // grab the head of the expression
        // arguably should use goto or an inner function
        do {
                res = nodeify_rvar(tokens, p);
                if (res.is_err) {
                        if (res.err.kind != NDFERR_NO_MATCH) { NDFERR_BAIL(res.err.kind); }
                } else {
                        head_idx = res.ok;
                        break;
                }

                res = nodeify_block(tokens, p);
                if (res.is_err) {
                        if (res.err.kind != NDFERR_NO_MATCH) { NDFERR_BAIL(res.err.kind); }
                } else {
                        head_idx = res.ok;
                        break;
                }

                // todo; if, while exprs.

                NDFERR_BAIL(NDFERR_NO_MATCH);

        } while (0);

        assert(head_idx != NO_NODE);

        while (1) {
                NodeifyResult res2 = nodeify_call(tokens, p, head_idx);
                if (res2.is_err) {
                        if (res2.err.kind != NDFERR_NO_MATCH) { NDFERR_BAIL(res2.err.kind); }
                        break;
                }
                head_idx = res2.ok;
        }
        return NodeifyResult_ok(head_idx);
}

NodeifyResult nodeify_place_expression(const TokenSlice tokens, Parser* p) {
        // TODO! currently just assumes place exprs are WORDs (i.e. variables as places)
        // needs e.g. dereferencing, array indexing

        Parser snapshot = *p;
        // peek(WORD)
        if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[p->cursor] != TOK_WORD) { NDFERR_BAIL(NDFERR_NO_MATCH); }
        // eat(SNK_WVAR)
        SynNode head = SynNode_new(SNK_WVAR, p->cursor);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*p, head, NodeifyResult_err(NDFERR_NOT_ENOUGH_NODES, __LINE__));
        p->cursor += 1;

        U4 head_idx = p->size - 1;
        return NodeifyResult_ok(head_idx);
}

NodeifyResult nodeify_assignment(const TokenSlice tokens, Parser* p, Bool is_args) {
        Parser snapshot = *p;
        // Success for the first two tokens, push head into the list and increment index
        SynNode head = SynNode_new(is_args ? SNK_KWARG : SNK_ASSIGNMENT, p->cursor);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*p, head, NodeifyResult_err(NDFERR_NOT_ENOUGH_NODES, __LINE__));
        
        U4 head_idx = p->size-1;
        U4 prev_idx;
        if (is_args) {
                // peek(WORD)
                if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
                if (tokens.tok_buf[p->cursor] != TOK_WORD) { NDFERR_BAIL(NDFERR_NO_MATCH); }
                // eat(SNK_WVAR)
                SynNode place = SynNode_new(SNK_WVAR, p->cursor);
                LIST_TRY_PUSH_WITH_EARLY_RETURN(*p, place, NodeifyResult_err(NDFERR_NOT_ENOUGH_NODES, __LINE__));
                p->cursor += 1;

                prev_idx = p->size - 1;
        } else {
                NodeifyResult res = nodeify_place_expression(tokens, p);
                if (res.is_err) { *p = snapshot; return res; }

                prev_idx = res.ok;
        }
        assert(p->buf[head_idx].next == NO_NODE);
        p->buf[head_idx].child = prev_idx;
        // peek(COLON | EQUAL)
        if (tokens.size < p->cursor+1) { NDFERR_BAIL(NDFERR_NOT_ENOUGH_TOKENS); }
        if (tokens.tok_buf[p->cursor] != (is_args ? TOK_COLON : TOK_EQUAL)) { NDFERR_BAIL(NDFERR_NO_MATCH); }
        // eat()
        p->cursor += 1;

        // Now nodeify the expression following
        NodeifyResult res = nodeify_expression(tokens, p);
        if (res.is_err) {
                *p = snapshot;
                return NodeifyResult_late(res);
        }
        // Connect prev to the nodeified expression.
        assert(p->buf[head_idx].next == NO_NODE);
        p->buf[prev_idx].next = res.ok;

        return NodeifyResult_ok(head_idx);
}

NodeifyResult nodeify(const TokenSlice tokens, Parser* p) {
        SynNode head = SynNode_new(SNK_PROGRAM, 0);
        LIST_TRY_PUSH_WITH_EARLY_RETURN(*p, head, NodeifyResult_err(NDFERR_NOT_ENOUGH_NODES, __LINE__));
        U4 head_idx = p->size-1;
        U4 prev_idx = NO_NODE;
        while (p->cursor < tokens.size) {
                NodeifyResult res = nodeify_assignment(tokens, p, false);
                if (res.is_err) {
                        if (res.err.kind == NDFERR_NO_MATCH) {
                                break;
                        }
                        return res;
                }
                // consume a semicolon after assignment
                // could be a NDFERR_BAIL
                if (tokens.size < p->cursor+1) { return NodeifyResult_err(NDFERR_NOT_ENOUGH_TOKENS, __LINE__); }
                if (tokens.tok_buf[p->cursor] != TOK_SEMICOLON) { return NodeifyResult_err(NDFERR_SYNTAX, __LINE__); }
                p->cursor += 1;
                prev_idx = Parser_append_child(p, head_idx, prev_idx, res.ok);
        }
        return NodeifyResult_ok(head_idx);
}



/*
*
* Ok, let's write out the grammar.
* PROGRAM: (ASSIGNMENT ';')*
* ASSIGNMENT: WORD '=' EXPRESSION ';'
* KWARG: WORD ':' EXPRESSION
* EXPRESSION: VALUE, BLOCK, ASSIGNMENT, INFIX
* INFIX: EXPRESSION OP EXPRESSION
* OP: '+' | '-' | '<' | '>' | '*' | '**' | '/' | '//'
* BLOCK: '{' (EXPRESSION (';' EXPRESSION)*)? '}'
* VALUE: WORD CALL* 
* CALL: '(' (EXPRESSION|KWARG) (, (EXPRESSION|KWARG)*) ')'
*
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
* main = Fn (
*       args = Args (
*               foo: I4,
*               bar: I8, 
*               baz: Str
*       )
*       ret = I4,
*       body = ( two = 2U4; four = 4U4; six = two.add(four); eight=two.add(six); six )
* )
*
* Str = struct (
*       size = U4
*       capacity = U4
*       buf = Ptr(U1)
* )
* Ptr = Fn(Type, body = ( VoidPtr ))
*
*/
