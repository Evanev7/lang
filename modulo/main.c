#include "std.h"
#include "stdlib.h"


typedef enum Token {
        TOK_EOF = 0,
        TOK_WORD,
        TOK_SPACE,
        TOK_COMMA,
        TOK_COLON,
        TOK_SEMICOLON,
        TOK_EQUAL,
        TOK_PLUS,
        TOK_MINUS,
        TOK_MUL,
        TOK_SLASH,
        TOK_LANGLE,
        TOK_RANGLE,
        TOK_LPAREN,
        TOK_RPAREN,
        TOK_LCURLY,
        TOK_RCURLY,
        TOK_LSQUARE,
        TOK_RSQUARE,
} Token;

#define TOK_CASE_DBG_NAME(tok) case tok: str->buf = String_buf_from_cstr(#tok); break;

Void Token_debug_name(String* str, Token token) {
        switch(token) {
                TOK_CASE_DBG_NAME(TOK_EOF);
                TOK_CASE_DBG_NAME(TOK_WORD);
                TOK_CASE_DBG_NAME(TOK_SPACE);
                TOK_CASE_DBG_NAME(TOK_COMMA);
                TOK_CASE_DBG_NAME(TOK_COLON);
                TOK_CASE_DBG_NAME(TOK_SEMICOLON);
                TOK_CASE_DBG_NAME(TOK_EQUAL);
                TOK_CASE_DBG_NAME(TOK_PLUS);
                TOK_CASE_DBG_NAME(TOK_MINUS);
                TOK_CASE_DBG_NAME(TOK_MUL);
                TOK_CASE_DBG_NAME(TOK_SLASH);
                TOK_CASE_DBG_NAME(TOK_LANGLE);
                TOK_CASE_DBG_NAME(TOK_RANGLE);
                TOK_CASE_DBG_NAME(TOK_LPAREN);
                TOK_CASE_DBG_NAME(TOK_RPAREN);
                TOK_CASE_DBG_NAME(TOK_LCURLY);
                TOK_CASE_DBG_NAME(TOK_RCURLY);
                TOK_CASE_DBG_NAME(TOK_LSQUARE);
                TOK_CASE_DBG_NAME(TOK_RSQUARE);
        }
}

typedef enum TokenizerError {
        TOKERR_NONE = 0,
        TOKERR_FULL_TOK_LIST,
} TokenizerError;

#define TOK_CASE(ch, tok) case ch: LIST_TRY_PUSH_WITH_EARLY_RETURN(*tokens, tok, TOKERR_FULL_TOK_LIST); break;
#define LIST_TRY_PUSH(list, item) if ((list).size < (list).capacity) {(list).buf[(list).size++] = item;}
#define LIST_TRY_PUSH_WITH_EARLY_RETURN(list, item, ret) if ((list).size < (list).capacity) { (list).buf[(list).size++] = item; } else { return ret; }

typedef struct TokenList {
        USize size;
        USize capacity;
        Token* buf;
} TokenList;

TokenList TokenList_new(const USize size) {
        return (TokenList) {
                .capacity = size,
                .size = 0,
                .buf = (Token*) malloc(size * sizeof(Token)),
        };
}

Void TokenList_pretty_print_debug(const TokenList tokens) {
        printf("TokenList(\n    size=%lu\n    capacity=%lu\n", tokens.size, tokens.capacity);
        String print_buf = String_from_cstr("                    ");
        for (U4 i = 0; i < tokens.size; i++) {
                printf("    %s\n", String_buf_to_cstr(print_buf.buf));
        }
        printf(")\n");
}

TokenizerError tokenize(const String text, TokenList* tokens) {
        U4 tok_idx = 0;
        for (U4 i = 0; i < text.size; i++) {
                if ((text.buf[i] == ' '
                    || text.buf[i] == '\n'
                    || text.buf[i] == '\r'
                    || text.buf[i] == '\t')
                ) {
                        if (tokens->buf[tokens->size] == TOK_SPACE) {
                                continue;
                        }
                        LIST_TRY_PUSH(*tokens, TOK_SPACE);
                        continue;
                }
                if (text.buf[i] == '_'
                        || (text.buf[i] >= 'a' && text.buf[i] <= 'z')
                        || (text.buf[i] >= 'A' && text.buf[i] <= 'Z')
                ) {
                        if (tokens->buf[tokens->size] == TOK_WORD) {
                                continue;
                        }
                        LIST_TRY_PUSH(*tokens, TOK_WORD);
                        continue;
                }
                switch (text.buf[i]) {
                        TOK_CASE(',', TOK_COMMA);
                        TOK_CASE(':', TOK_COLON);
                        TOK_CASE(';', TOK_SEMICOLON);
                        TOK_CASE('=', TOK_EQUAL);
                        TOK_CASE('+', TOK_PLUS);
                        TOK_CASE('-', TOK_MINUS);
                        TOK_CASE('*', TOK_MUL);
                        TOK_CASE('/', TOK_SLASH);
                        TOK_CASE('<', TOK_LANGLE);
                        TOK_CASE('>', TOK_RANGLE);
                        TOK_CASE('(', TOK_LPAREN);
                        TOK_CASE(')', TOK_RPAREN);
                        TOK_CASE('{', TOK_LCURLY);
                        TOK_CASE('}', TOK_RCURLY);
                        TOK_CASE('[', TOK_LSQUARE);
                        TOK_CASE(']', TOK_RSQUARE);
                }
        }
        LIST_TRY_PUSH(*tokens, TOK_EOF);
        return TOKERR_NONE;
}

I4 main(I4 argc, char** argv) {
        String code = String_from_cstr("hello<with = foo>");
        TokenList toks = TokenList_new(100);
        TokenizerError err = tokenize(code, &toks);
        if (err) {
                return err;
        }

        TokenList_pretty_print_debug(toks);
        return 0;
}
