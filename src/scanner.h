#pragma once

typedef enum {
    TOK_ERR,
    TOK_EOF,
    TOK_EOL,
    TOK_CMD,
    TOK_STR,
    TOK_INT,
    TOK_VAR,
    TOK_LBRACE,
    TOK_RBRACE,
} TokenType;

// TOK_STR, TOK_CMD, TOK_VAR contain allocated data, the rest do not
typedef struct {
    TokenType type;
    union {
        const char* str;
        long num;
    } as;
} Token;

typedef struct {
    char* start;
    char* current;
    bool eof;
} Scanner;

Scanner init_scanner(char* src);
Token scanner_next(Scanner* sc);

void token_free(Token* tok);
