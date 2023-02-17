#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "eval.h"
#include "parser.h"
#include "scanner.h"

#define LINE_LEN 1024
#define PROMPT "[%ld]sysh$ "
#define EPROMPT "[E]sysh$ "

void repl() {
    char buf[LINE_LEN];
    printf(PROMPT, 0L);
    while(fgets(buf, LINE_LEN, stdin)) {
        Scanner sc = init_scanner(buf);
        BlockResult br = parse(&sc);
        if(!br.is_ok) {
            printf("sysh: %s\n", br.as.err);
            printf(EPROMPT);
        } else if(br.as.ok.len > 0) {
            long result = eval_block(&br.as.ok);
            printf(PROMPT, result);
            block_free(&br.as.ok);
        }
    }
}

int main(void) {
    repl();
    return 0;
}
