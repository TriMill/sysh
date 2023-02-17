#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "eval.h"
#include "parser.h"
#include "scanner.h"

static long eval_syscall(Line* line) {
    if(line->len > 6) {
        fprintf(stderr, "sysh: too many args for syscall\n");
        return 0;
    }
    long args[6] = {0,0,0,0,0,0};
    bool cloned[6] = {0,0,0,0,0,0};
    for(int i = 0; i < line->len; i++) {
        Argument arg = line->args[i];
        if(arg.type == ARG_NUM) {
            args[i] = arg.as.num;
        } else if(arg.type == ARG_STR) {
            int len = strlen(arg.as.str);
            char* buf = malloc(len + 1);
            strcpy(buf, arg.as.str);
            args[i] = (long)buf;
            cloned[i] = true;
        } else {
            for(int j = 0; j < line->len; j++) {
                if(cloned[j]) free((void*)args[j]);
            }
            fprintf(stderr, "sysh: invalid arg type\n");
            return 0;
        }
    }
    long result = syscall(line->id, args[0], args[1], args[2], args[3], args[4], args[5]);
    for(int i = 0; i < line->len; i++) {
        if(cloned[i]) free((void*)args[i]);
    }
    return result;
}

static long eval_line(Line* line) {
    if(line->id >= 0) {
        return eval_syscall(line);
    } else {
        // TODO
        return 0;
    }
}


long eval_block(Block* block) {
    long result = 0;
    for(int i = 0; i < block->len; i++) {
        result = eval_line(&block->lines[i]);
        if(errno > 0) {
            fprintf(stderr, "E%d: %s\n", errno, strerror(errno));
        } 
        break;
    }
    return result;
}
