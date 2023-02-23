#pragma once
#include <stdbool.h>

#define TABLE_MAX_LOAD 0.75

typedef struct {
    const char* key;
    long value;
} Entry;

typedef struct {
    int len;
    int capacity;
    Entry* entries;
} Hashmap;

void hashmap_init(Hashmap* hashmap);
void hashmap_free(Hashmap* hashmap);

bool hashmap_add(Hashmap* hashmap, const char* key, long value);
bool hashmap_get(Hashmap* hashmap, const char* key, long* value);
bool hashmap_remove(Hashmap* hashmap, const char* key);

