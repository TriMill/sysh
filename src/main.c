#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "hashmap.h"
#include "parser.h"
#include "scanner.h"

#define LINE_LEN 1024
#define PROMPT "[%ld]sysh$ "
#define EPROMPT "[E]sysh$ "

static long repl() {
    char buf[LINE_LEN];
    printf(PROMPT, 0L);
    Hashmap vars;
    hashmap_init(&vars);
    while(fgets(buf, LINE_LEN, stdin)) {
        Scanner sc = init_scanner(buf);
        BlockResult br = parse(&sc);
        if(!br.is_ok) {
            printf("sysh: %s\n", br.as.err);
            printf(EPROMPT);
        } else if(br.as.ok.len > 0) {
            long result = eval_block(&br.as.ok, &vars);
            printf(PROMPT, result);
            block_free(&br.as.ok);
        }
    }
    hashmap_free(&vars);
    return 0;
}

static long run_file(const char* name) {
    FILE* file = fopen(name, "r");
    if(file == NULL) {
        fprintf(stderr, "%s\n", strerror(errno));
        return 1;
    }
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buf = malloc(fsize + 1);
    fread(buf, fsize, 1, file);
    fclose(file);

    buf[fsize] = '\0';

    Hashmap vars;
    hashmap_init(&vars);
    Scanner sc = init_scanner(buf);
    BlockResult br = parse(&sc);
    if(!br.is_ok) {
        printf("sysh: %s\n", br.as.err);
    } else if(br.as.ok.len > 0) {
        eval_block(&br.as.ok, &vars);
        block_free(&br.as.ok);
    }
    hashmap_free(&vars);
    free(buf);

    return 0;
}

int main(int argc, const char** argv) {
    if(argc < 1) return 1;
    if(argc == 1) return repl();
    if(argc == 2) return run_file(argv[1]);
    fprintf(stderr, "usage: %s [file]\n", argv[0]);
    return 1;
}
