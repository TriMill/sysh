#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "hashmap.h"

static uint32_t hash_string(const char* key) {
    uint32_t hash = 2166136261u;
    while(*key != '\0') {
        hash ^= (uint8_t)(*key);
        hash *= 16777619;
        key++;
    }
    return hash;
}

void hashmap_init(Hashmap* hashmap) {
    hashmap->len = 0;
    hashmap->capacity = 0;
    hashmap->entries = NULL;
}

void hashmap_free(Hashmap* hashmap) {
    for(int i = 0; i < hashmap->capacity; i++) {
        Entry e = hashmap->entries[i];
        if(e.key != NULL) free((char*)e.key);
    }
    free(hashmap->entries);
    hashmap_init(hashmap);
}

static Entry* hashmap_find(Entry* entries, int capacity, const char* key) {
    uint32_t index = hash_string(key) % capacity;
    Entry* tombstone = NULL;
    while(true) {
        Entry* entry = &entries[index];
        if(entry->key == NULL) {
            if(entry->value == 1) {
                if(tombstone == NULL) tombstone = entry;
            } else {
                return (tombstone == NULL) ? entry : tombstone;
            }
        } else if(strcmp(entry->key, key) == 0) {
            return entry;
        }
    }
}

static void hashmap_grow(Hashmap* map, int capacity) {
    Entry* entries = malloc(capacity * sizeof(Entry));
    for(int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = 0;
    }
    map->len = 0;
    for(int i = 0; i < map->capacity; i++) {
        Entry* entry = &map->entries[i];
        if(entry->key == NULL) continue;
        
        Entry* dest = hashmap_find(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        map->len++;
    }
    free(map->entries);

    map->entries = entries;
    map->capacity = capacity;
}

bool hashmap_get(Hashmap* hashmap, const char* key, long* value) {
    if(hashmap->len == 0) return false;

    Entry* e = hashmap_find(hashmap->entries, hashmap->capacity, key);
    if(e->key == NULL) return false;

    *value = e->value;
    return true;
}

bool hashmap_add(Hashmap* hashmap, char* key, long value) {
    if(hashmap->len + 1 > hashmap->capacity * TABLE_MAX_LOAD) {
        int capacity = (hashmap->capacity == 0 ? 8 : (2 * hashmap->capacity));
        hashmap_grow(hashmap, capacity);
    }
    Entry* e = hashmap_find(hashmap->entries, hashmap->capacity, key);
    bool new_key = e->key == NULL;
    if(new_key) {
        hashmap->len++;
    }
    e->key = key;
    e->value = value;                
    return new_key;
}

bool hashmap_remove(Hashmap* hashmap, const char* key) {
    if(hashmap->len == 0) return false;

    Entry* e = hashmap_find(hashmap->entries, hashmap->capacity, key);
    if(e->key == NULL) return false;

    e->key = NULL;
    e->value = 1;
    return true;
}
