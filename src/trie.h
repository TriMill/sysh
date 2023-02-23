#pragma once

#define C_ALLOC     -2
#define C_REALLOC   -3
#define C_FREE      -4
#define C_SET       -5
#define C_CPY       -6
#define C_DEREF     -7
#define C_IF        -8
#define C_WHILE     -9
#define C_ADD       -10
#define C_SUB       -11
#define C_MUL       -12
#define C_DIV       -13


long trie_get(const char* key);
