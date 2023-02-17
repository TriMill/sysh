#pragma once

#include <stdbool.h>
#include "scanner.h"

#define RESULT(T, E) struct { bool is_ok; union { T ok; E err; } as; }
#define OK(val, R) (R){.is_ok = 1, .as.ok = (val) }
#define ERR(val, R) (R){.is_ok = 0, .as.err = (val) }

typedef struct Argument_s Argument;

typedef struct {
    long id;
    int len;
    int capacity;
    Argument* args;
} Line;

typedef struct {
    int len;
    int capacity;
    Line* lines;
} Block;

typedef enum {
    ARG_BLOCK,
    ARG_STR,
    ARG_NUM,
    ARG_VAR,
    ARG_CMD,
} ArgType;

struct Argument_s {
    ArgType type;
    union {
        Block block;
        const char* str;
        long num;
    } as;
};

typedef RESULT(Block, const char*) BlockResult;
typedef RESULT(Line, const char*) LineResult;

void block_init(Block* b);
void block_add(Block* b, Line l);
void block_free(Block* b);

void line_init(Line* l, long id);
void line_add(Line* l, Argument a);
void line_free(Line* l);

BlockResult parse(Scanner* sc);
