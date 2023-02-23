#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

#include "eval.h"
#include "hashmap.h"
#include "parser.h"
#include "scanner.h"
#include "trie.h"

static void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "sysh: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
}

bool eval_arg(Argument arg, long* result, bool* cloned, Hashmap* vars) {
    if(arg.type == ARG_NUM) {
        *result = arg.as.num;
        if(cloned) *cloned = false;
        return true;
    } else if(arg.type == ARG_STR && cloned != NULL) {
        int len = strlen(arg.as.str);
        char* buf = malloc(len + 1);
        strcpy(buf, arg.as.str);
        *result = (long)buf;
        *cloned = true;
        return true;
    } else if(arg.type == ARG_BLOCK) {
        *result = eval_block(&arg.as.block, vars);        
        if(cloned) *cloned = false;
        return true;
    } else if(arg.type == ARG_VAR) {
        if(hashmap_get(vars, arg.as.str, result)) {
            if(cloned) *cloned = false;
            return true;
        } else {
            return false;
        }
    } {
        return false;
    }
}

static long eval_syscall(Line* line, Hashmap* vars) {
    if(line->len > 6) {
        log_error("too many arguments for syscall: got %d", line->len);
        return -1;
    }
    long args[6] = {0,0,0,0,0,0};
    bool cloned[6] = {0,0,0,0,0,0};
    for(int i = 0; i < line->len; i++) {
        Argument arg = line->args[i];
        if(!eval_arg(arg, &args[i], &cloned[i], vars)) {
            for(int j = 0; j < line->len; j++) {
                if(cloned[j]) free((void*)args[j]);
            }
            log_error("bad argument to syscall");
            return -1;
        }
    }
    long result = syscall(line->id, args[0], args[1], args[2], args[3], args[4], args[5]);
    hashmap_add(vars, "ERRNO", errno);
    for(int i = 0; i < line->len; i++) {
        if(cloned[i]) free((void*)args[i]);
    }
    return result;
}

static long eval_alloc(Line* line, Hashmap* vars) {
    if(line->len != 1) {
        log_error(".alloc expected 1 arg, got %d", line->len);
        return -1;
    }
    long val;
    if(!eval_arg(line->args[0], &val, NULL, vars)) {
        log_error("bad argument to .alloc");
        return -1;
    }
    return (long)malloc(val);
}

static long eval_realloc(Line* line, Hashmap* vars) {
    if(line->len != 2) {
        log_error(".realloc expected 2 args, got %d", line->len);
        return -1;
    }
    long val1;
    long val2;
    if(!eval_arg(line->args[0], &val1, NULL, vars)) {
        log_error("bad argument to .realloc");
        return -1;
    }
    if(!eval_arg(line->args[1], &val2, NULL, vars)) {
        log_error("bad argument to .realloc");
        return -1;
    }
    return (long)realloc((void*)val1, val2);
}

static long eval_free(Line* line, Hashmap* vars) {
    if(line->len != 1) {
        log_error(".free expected 1 args, got %d", line->len);
        return -1;
    }
    long val;
    if(!eval_arg(line->args[0], &val, NULL, vars)) {
        log_error("bad argument to .free");
        return -1;
    }
    free((void*)val);
    return 0;
}

static long eval_set(Line* line, Hashmap* vars) {
    if(line->len < 0 || line->len > 2) {
        log_error(".set expected 1 or 2 args, got %d", line->len);
        return -1;
    }
    if(line->args[0].type != ARG_VAR) {
        log_error("bad argument to .set");
        return -1;
    }
    if(line->len == 2) {
        long val;
        bool cloned;
        if(!eval_arg(line->args[1], &val, &cloned, vars)) {
            log_error("bad argument to .set");
            return -1;
        }
        hashmap_add(vars, line->args[0].as.str, val);
    } else {
        hashmap_remove(vars, line->args[0].as.str);
    }
    return 0;
}

static long eval_while(Line* line, Hashmap* vars) {
    if(line->len != 2) {
        log_error(".while expected 2 args, got %d", line->len);
        return -1;
    }
    long result = 0;
    while(true) {
        long val;
        if(!eval_arg(line->args[0], &val, NULL, vars)) {
            log_error("bad argument to .while");
            return -1;
        }
        if(!val) break;
        if(!eval_arg(line->args[1], &result, NULL, vars)) {
            log_error("bad argument to .while");
            return -1;
        }
    }
    return result;
}

static long eval_if(Line* line, Hashmap* vars) {
    if(line->len < 2 || line->len > 3) {
        log_error(".if expected 2 or 3 args, got %d", line->len);
        return -1;
    }
    long val;
    if(!eval_arg(line->args[0], &val, NULL, vars)) {
        log_error("bad argument to .if");
        return -1;
    }
    if(val) {
        long result;
        if(!eval_arg(line->args[1], &result, NULL, vars)) {
            log_error("bad argument to .if");
            return -1;
        }
        return result;
    } else if(line->len == 3) {
        long result;
        if(!eval_arg(line->args[2], &result, NULL, vars)) {
            log_error("bad argument to .if");
            return -1;
        }
        return result;
        
    } else {
        return 0;
    }
}

static long eval_cpy(Line* line, Hashmap* vars) {
    if(line->len != 3) {
        log_error(".cpy expected 3 arguments, got %d", line->len);
        return -1;
    }
    long dst;
    if(!eval_arg(line->args[0], &dst, NULL, vars)) {
        log_error("bad argument to .cpy");
        return -1;
    }
    long n;
    if(!eval_arg(line->args[2], &n, NULL, vars)) {
        log_error("bad argument to .cpy");
        return -1;
    }
    long src;
    bool cloned;
    if(!eval_arg(line->args[1], &src, &cloned, vars)) {
        log_error("bad argument to .cpy");
        return -1;
    }
    memcpy((void*)dst, (void*)src, n);
    if(cloned) free((void*)src);
    return 0;
}

static long eval_deref(Line* line, Hashmap* vars) {
    if(line->len != 1) {
        log_error(".deref expected 1 argument, got %d", line->len);
        return -1;
    }
    long val;
    bool cloned;
    if(!eval_arg(line->args[0], &val, &cloned, vars)) {
        log_error("bad argument to .deref");
        return -1;
    }
    long result = *((unsigned char*)val);
    if(cloned) free((void*)val);
    return result;
}

static long fn_add(long a, long b) { return a + b; }
static long fn_sub(long a, long b) { return a - b; }
static long fn_mul(long a, long b) { return a * b; }
static long fn_div(long a, long b) { return a / b; }

static long eval_op(Line* line, Hashmap* vars, const char* name, long (*op)(long, long)) {
    if(line->len != 2) {
        log_error("%s expected 1 argument, got %d", name, line->len);
        return -1;
    }
    long val1;
    if(!eval_arg(line->args[0], &val1, NULL, vars)) {
        log_error("bad argument to %s", name);
        return -1;
    }
    long val2;
    if(!eval_arg(line->args[1], &val2, NULL, vars)) {
        log_error("bad argument to %s", name);
        return -1;
    }
    return op(val1, val2);
}

static long eval_line(Line* line, Hashmap* vars) {
    if(line->id >= 0) {
        return eval_syscall(line, vars);
    } else switch(line->id) {
        case C_ALLOC:    return eval_alloc(line, vars);
        case C_REALLOC:  return eval_realloc(line, vars);
        case C_FREE:     return eval_free(line, vars);
        case C_SET:      return eval_set(line, vars);
        case C_CPY:      return eval_cpy(line, vars);
        case C_DEREF:    return eval_deref(line, vars);
        case C_WHILE:    return eval_while(line, vars);
        case C_IF:       return eval_if(line, vars);
        case C_ADD:      return eval_op(line, vars, ".add", fn_add);
        case C_SUB:      return eval_op(line, vars, ".sub", fn_sub);
        case C_MUL:      return eval_op(line, vars, ".mul", fn_mul);
        case C_DIV:      return eval_op(line, vars, ".div", fn_div);
        default: return 0; // unreachable
    }
}

long eval_block(Block* block, Hashmap* vars) {
    long result = 0;
    for(int i = 0; i < block->len; i++) {
        result = eval_line(&block->lines[i], vars);
        hashmap_add(vars, "LAST", result);
        if(errno > 0) {
            log_error("E%d: %s", errno, strerror(errno));
        } 
    }
    return result;
}
