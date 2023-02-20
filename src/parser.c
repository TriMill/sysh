#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "scanner.h"
#include "trie.h"

void block_init(Block* b) {
    b->len = 0;
    b->capacity = 0;
    b->lines = NULL;
}

void block_add(Block* b, Line l) {
    if(b->capacity <= b->len) {
        int new_capacity = (b->capacity == 0 ? 8 : 2*(b->capacity));
        b->lines = realloc(b->lines, new_capacity * sizeof(Line));
        b->capacity = new_capacity;
    }
    b->lines[b->len] = l;
    b->len++;
}

void block_free(Block* b) {
    for(int i = 0; i < b->len; i++) {
        line_free(&b->lines[i]);
    }
    free(b->lines);
    block_init(b);
}

void line_init(Line* l, long id) {
    l->id = id;
    l->len = 0;
    l->capacity = 0;
    l->args = NULL;
}

void line_add(Line* l, Argument arg) {
    if(l->capacity <= l->len) {
        int new_capacity = (l->capacity == 0 ? 8 : 2*(l->capacity));
        l->args = realloc(l->args, new_capacity * sizeof(Argument));
        l->capacity = new_capacity;
    }
    l->args[l->len] = arg;
    l->len++;
}

void line_free(Line* line) {
    for(int i = 0; i < line->len; i++) {
        switch(line->args[i].type) {
            case ARG_BLOCK:
                block_free(&line->args[i].as.block);
                break;
            case ARG_STR:
            case ARG_CMD:
            case ARG_VAR:
                free((char*)line->args[i].as.str);
                break;
            case ARG_NUM:
                break;
        }
    }
    free(line->args);
}

static BlockResult parse_block(Scanner* sc, bool braced);

static LineResult parse_line(Scanner* sc, int id, bool* brace_end) {
    Line line;
    line_init(&line, id);
    while(true) {
        Token tok = scanner_next(sc);
        switch(tok.type) {
            case TOK_EOF:
            case TOK_EOL:
                *brace_end = false;
                return OK(line, LineResult);
            case TOK_RBRACE:
                *brace_end = true;
                return OK(line, LineResult);
            case TOK_ERR: 
                line_free(&line);
                return ERR(tok.as.str, LineResult);
            case TOK_INT:
                line_add(&line, (Argument){.type = ARG_NUM, .as.num = tok.as.num});
                break;
            case TOK_VAR:
                line_add(&line, (Argument){.type = ARG_VAR, .as.str = tok.as.str});
                break;
            case TOK_STR: 
                line_add(&line, (Argument){.type = ARG_STR, .as.str = tok.as.str});
                break;
            case TOK_CMD:
                line_add(&line, (Argument){.type = ARG_CMD, .as.str = tok.as.str});
                break;
            case TOK_LBRACE: {
                BlockResult br = parse_block(sc, true);
                if(!br.is_ok) {
                    line_free(&line);
                    return ERR(br.as.err, LineResult);
                }
                line_add(&line, (Argument){.type = ARG_BLOCK, .as.block = br.as.ok});
            } break;
            default:
                line_free(&line);
                return ERR("unexpected token in line: %d", LineResult);
        }
    }
}

static BlockResult parse_block(Scanner* sc, bool braced)  {
    Block block;
    block_init(&block);
    while(true) {
        Token tok = scanner_next(sc);
        if((!braced && tok.type == TOK_EOF) || (braced && tok.type == TOK_RBRACE)) {
            return OK(block, BlockResult);
        }
        switch(tok.type) {
            case TOK_ERR: 
                block_free(&block);
                return ERR(tok.as.str, BlockResult);
            case TOK_EOL: 
                continue;
            case TOK_CMD: {
                long id = trie_get(tok.as.str);
                token_free(&tok);
                if(id == -1) {
                    block_free(&block);
                    return ERR("invalid syscall or command name", BlockResult);
                }
                bool brace_end;
                LineResult sr = parse_line(sc, id, &brace_end);
                if(!sr.is_ok) {
                    block_free(&block);
                    return ERR(sr.as.err, BlockResult);
                }
                if(brace_end && !braced) {
                    line_free(&sr.as.ok);
                    block_free(&block);
                    return ERR("unexpected token in block", BlockResult);
                }
                block_add(&block, sr.as.ok);
                if(brace_end) {
                    return OK(block, BlockResult);
                }
            } break;
            default: 
                block_free(&block);
                return ERR("unexpected token in block", BlockResult);
        }
    }
    return OK(block, BlockResult);
}

BlockResult parse(Scanner* sc) {
    return parse_block(sc, false);
}
