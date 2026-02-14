#include "std.h"
#include <stdlib.h>

typedef enum Token {
        TOK_EOF = 0,
        TOK_WORD,
        TOK_SPACE,
        TOK_COMMA,
        TOK_SEMICOLON,
        TOK_COLON,
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
        TOK_LOOP,
        TOK_BREAK,
        TOK_CONTINUE,
        TOK_IF,
} Token;

StringSlice KW_IF = StringSlice_from_cstr("if");
StringSlice KW_LOOP = StringSlice_from_cstr("loop");
StringSlice KW_BREAK = StringSlice_from_cstr("break");
StringSlice KW_CONTINUE = StringSlice_from_cstr("continue");

#define TOK_CASE_DBG_NAME(tok) case tok: return U1_ptr_from_cstr(#tok);

U1* Token_retrieve_debug_name(Token token) {
        switch(token) {
                TOK_CASE_DBG_NAME(TOK_EOF);
                TOK_CASE_DBG_NAME(TOK_WORD);
                TOK_CASE_DBG_NAME(TOK_SPACE);
                TOK_CASE_DBG_NAME(TOK_COMMA);
                TOK_CASE_DBG_NAME(TOK_SEMICOLON);
                TOK_CASE_DBG_NAME(TOK_COLON);
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
                TOK_CASE_DBG_NAME(TOK_LOOP);
                TOK_CASE_DBG_NAME(TOK_BREAK);
                TOK_CASE_DBG_NAME(TOK_CONTINUE);
                TOK_CASE_DBG_NAME(TOK_IF);
        }
        return U1_ptr_from_cstr("TOK_UNKNOWN");
}

typedef enum TokenizerError {
        TOKERR_NONE = 0,
        TOKERR_FULL_TOK_LIST,
} TokenizerError;

#define TOKERR_CASE_DBG_NAME(tok) case tok: return U1_ptr_from_cstr(#tok);

U1* TokenizerError_retrieve_debug_name(TokenizerError tok_err) {
        switch(tok_err) {
                TOKERR_CASE_DBG_NAME(TOKERR_NONE);
                TOKERR_CASE_DBG_NAME(TOKERR_FULL_TOK_LIST);
        };
        return U1_ptr_from_cstr("TOKERR_UNKNOWN");
}
Void TokenizerError_pretty_print_debug(TokenizerError tok_err) {
        printf("TokenizerError(%s)\n", TokenizerError_retrieve_debug_name(tok_err));
}

typedef struct TokenList {
        Token* tok_buf;
        USize* idx_buf;
        USize* size_buf;
        USize size;
        USize capacity;
} TokenList;

TokenList TokenList_new(const USize capacity) {
        return (TokenList) {
                .capacity = capacity,
                .size = 0,
                .idx_buf = (USize*) malloc(capacity * sizeof(USize)),
                .tok_buf = (Token*) malloc(capacity * sizeof(Token)),
                .size_buf = (USize*) malloc(capacity * sizeof(USize)),
        };
}

typedef struct TokenSlice {
        Token* tok_buf;
        USize* idx_buf;
        USize* size_buf;
        USize size;
} TokenSlice;

TokenSlice TokenSlice_from_list(const TokenList tokens) {
        return (TokenSlice) {
                .tok_buf = tokens.tok_buf,
                .idx_buf = tokens.idx_buf,
                .size_buf = tokens.size_buf,
                .size = tokens.size
        };
}

Void TokenList_pretty_print_debug(const TokenList tokens) {
        printf("TokenList(\n    size=%lu,\n    capacity=%lu,\n    tokens=[\n", tokens.size, tokens.capacity);
        for (USize i = 0; i < tokens.size; i+=1) {
                printf("        (%4lu, %4lu): %s,\n", tokens.idx_buf[i], tokens.size_buf[i], U1_ptr_to_cstr(Token_retrieve_debug_name(tokens.tok_buf[i])));
        }
        printf("])\n");
}

Token classify_word(const StringSlice word) {
        if (StringSlice_equal(word, KW_IF)) {
                return TOK_IF;
        }
        if (StringSlice_equal(word, KW_LOOP)) {
                return TOK_LOOP;
        }
        if (StringSlice_equal(word, KW_BREAK)) {
                return TOK_BREAK;
        }
        if (StringSlice_equal(word, KW_CONTINUE)) {
                return TOK_CONTINUE;
        }
        
        return TOK_WORD;
}

Bool U1_is_whitespace(U1 character) {
        return character == ' '
                    || character == '\n'
                    || character == '\r'
                    || character == '\t';

}

Bool U1_is_text(U1 character) {
        return character == '_'
                || (character >= 'a' && character <= 'z')
                || (character >= 'A' && character <= 'Z');
}

Bool Token_is_sep(Token tok) {
        return tok == TOK_COMMA || tok == TOK_SEMICOLON;
}

TokenizerError TokenList_push(TokenList* tokens, Token tok, U4 idx, U4 size) {
        if (tokens->size >= tokens->capacity) {
                return TOKERR_FULL_TOK_LIST;
        }
        USize i = tokens->size;
        tokens->tok_buf[i] = tok;
        tokens->idx_buf[i] = idx;
        tokens->size_buf[i] = size;
        tokens->size += 1;
        return TOKERR_NONE;
}

#define TOK_PUSH(tok, idx) do { TokenizerError err = TokenList_push(tokens, tok, idx, tok_len); tok_len = 0; if (err) {return err;} } while (0)
#define TOK_CASE(ch, tok) case ch: TOK_PUSH(tok,i); continue;

TokenizerError tokenize(const StringSlice text, TokenList* tokens) {
        U4 tok_idx = 0;
        U4 tok_len = 0;
        for (U4 i = 0; i < text.size; i+=1) {
                tok_len += 1;
                // if this is text and the next char is not text (1 char lookahead), push a WORD
                if (U1_is_text(text.buf[i]) && (i+1 >= text.size || !U1_is_text(text.buf[i+1]))) {
                        // just checking my algebra:
                        // when tokenizing `loop`
                        // tok_len is 4, i is 3. i + 1 >= text.size, so we hit this branch
                        // we classify_word on (StringSlice) { .size=4, .buf=&text.buf[3+1-4]}
                        Token tok = classify_word((StringSlice) {.size=tok_len, .buf=&text.buf[i+1-tok_len] });
                        TOK_PUSH(tok, i+1-tok_len);
                        continue;
                } 
                // if this is whitespace and next isn't, push a SPACE
                if (U1_is_whitespace(text.buf[i]) && (i+1 >= text.size || !U1_is_whitespace(text.buf[i+1]))) {
                        // if we aren't pushing a token we should reset the tok_len
                        tok_len = 0;
                        //TOK_PUSH(TOK_SPACE, i);
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
        TOK_PUSH(TOK_EOF, text.size);
        return TOKERR_NONE;
}

