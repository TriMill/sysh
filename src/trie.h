#pragma once

#define C_ALLOC     -2
#define C_REALLOC   -3
#define C_FREE      -4
#define C_SET       -5
#define C_CPY       -6
#define C_DEREF     -7
#define C_STRERROR  -8
#define C_IF        -9
#define C_WHILE     -10
#define C_FOR       -11

long trie_get(const char* key);
