#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "scanner.h"

// Based heavily on the scanner implementation 
// from Crafting Interpreters by Robert Nystrom

Scanner init_scanner(char *src) {
    return (Scanner){.start=src, .current=src, .eof=(*src == '\0')};
}

void token_free(Token* tok) {
    if(tok->type == TOK_STR || tok->type == TOK_CMD || tok->type == TOK_VAR) {
        free((char*)(tok->as.str));
    }
}

static char peek(const Scanner* sc) {
    return *sc->current;
}

static char next(Scanner* sc) {
    if(*sc->current == '\0') {
        sc->eof = true;
        return '\0';
    }
    char c = *sc->current;
    sc->current++;
    return c;
}

static bool is_alnum(char c) {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9') || c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static Token err_token(const char* msg) {
    Token token = {
        .type = TOK_ERR, 
        .as.str = msg,
    };
    return token;
}

static void skip_ws(Scanner* sc) {
    while(true) {
        char c = peek(sc);
        switch(c) {
            case ' ': 
            case '\t': 
                next(sc);
                break;
            case '#':
                while(peek(sc) != '\n' && peek(sc) != '\0') {
                    next(sc);
                }
                return;
            default:
                return;
        }
    }
}

static Token scan_string(Scanner* sc) {
    while(peek(sc) != '\0' && peek(sc) != '\'') next(sc);
    if(peek(sc) == '\0') return err_token("EOF while scanning raw string");
    next(sc);

    int len = sc->current - sc->start - 2;
    char* buf = malloc((len + 1) * sizeof(char));
    memcpy(buf, sc->start + 1, len);
    buf[len] = '\0';
    return (Token){
        .type = TOK_STR,
        .as.str = buf,
    };
}

static char* add_char(char* buf, int* len, int* capacity, char new) {
    if(*len == *capacity) {
        int new_capacity = (*capacity == 0 ? 8 : 2*(*capacity));
        buf = realloc(buf, new_capacity);
        *capacity = new_capacity;
    }
    buf[*len] = new;
    (*len)++;
    return buf;
}

static Token scan_escape_string(Scanner* sc) {
    char* buf = NULL;
    int len = 0;
    int capacity = 0;
    char c;
    while(true) {
        c = next(sc);
        if(c == '"') break;
        if(c == '\0') {
            free(buf);
            return err_token("EOF while scanning double-quoted string");
        }
        if(c == '\\') {
            switch(next(sc)) {
                case '\\': buf = add_char(buf, &len, &capacity, '\\'); break;
                case '"':  buf = add_char(buf, &len, &capacity, '"'); break;
                case 'n':  buf = add_char(buf, &len, &capacity, '\n'); break;
                case 'r':  buf = add_char(buf, &len, &capacity, '\r'); break;
                case 't':  buf = add_char(buf, &len, &capacity, '\t'); break;
                case '0':  buf = add_char(buf, &len, &capacity, '\0'); break;
                default: {
                    free(buf);
                    return err_token("unknown escape sequence");
                }
            }
        } else {
            buf = add_char(buf, &len, &capacity, c);
        }
    }

    buf = add_char(buf, &len, &capacity, '\0');
    buf = realloc(buf, len);

    return (Token){
        .type = TOK_STR,
        .as.str = buf,
    };
}

static Token scan_var(Scanner* sc) {
    while(is_alnum(peek(sc))) next(sc);
    
    int len = sc->current - sc->start - 1;
    char* buf = malloc((len + 1) * sizeof(char));
    memcpy(buf, sc->start + 1, len);
    buf[len] = '\0';
    return (Token){
        .type = TOK_VAR,
        .as.str = buf,
    };
}

static Token scan_cmd(Scanner* sc) {
    while(is_alnum(peek(sc))) next(sc);

    int len = sc->current - sc->start;
    char* buf = malloc((len + 1) * sizeof(char));
    memcpy(buf, sc->start, len);
    buf[len] = '\0';
    return (Token){
        .type = TOK_CMD,
        .as.str = buf,
    };
}

static Token scan_num(Scanner* sc) {
    while(is_digit(peek(sc))) next(sc);
    // TODO base
    int len = sc->current - sc->start;
    char buf[len+1];
    memcpy(buf, sc->start, len);
    buf[len] = '\0';
    long num = strtol(buf, NULL, 10);
    return (Token){
        .type = TOK_INT,
        .as.num = num,
    };
}

Token scanner_next(Scanner* sc) {
    skip_ws(sc);
    sc->start = sc->current;
    char c = next(sc);
    if(c == '-' || is_digit(c)) {
        return scan_num(sc);
    }
    if(c == '.' || is_alnum(c)) {
        return scan_cmd(sc);
    }
    switch(c) {
        case '\0': return (Token){.type = TOK_EOF};
        case '\n': 
        case ';': return (Token){.type = TOK_EOL};
        case '{': return (Token){.type = TOK_LBRACE};
        case '}': return (Token){.type = TOK_RBRACE};
        case '$': return scan_var(sc);
        case '\'': return scan_string(sc);
        case '\"': return scan_escape_string(sc);
        default: return err_token("Unexpected character");
    }
}
