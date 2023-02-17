#!/usr/bin/env python

# Python script to generate a trie using switch statements in C

import sys
import re

if len(sys.argv) < 3:
    print("Not enough arguments. Usage: triegen.py <input> <output>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

with open(input_file, 'r') as f:
    data = [re.split('\s+', l.strip()) for l in f.read().split('\n') if len(l.strip()) > 0]

trie = {}

for line in data:
    key = line[0] + '\0'
    val = int(line[1])
    trie_local = trie
    for c in key:
        if c == '\0':
            trie_local[c] = val
        elif trie_local.get(c) != None:
            trie_local = trie_local[c]
        else:
            trie_local[c] = {}
            trie_local = trie_local[c]

def matches_exact(trie, start):
    if len(trie) != 1:
        return False
    if trie.get('\0') != None:
        if(start):
            return False
        return ('', trie.get('\0'))
    k, v = list(trie.items())[0]
    res = matches_exact(v, False)
    if res == False:
        return False
    return (k + res[0], res[1])

def write_trie(f, trie, depth):
    if line := matches_exact(trie, True):
        if len(line[0]) == 1:
            f.write('if(key[%s] == \'%s\') { return %s; } break;\n' % (depth, line[0], line[1]))
        else:
            f.write('if(strcmp(key + %s, "%s") == 0) { return %s; } break;\n' % (depth, line[0], line[1]))
        return
    ws = "  " * (depth + 2)
    f.write("switch(key[%s]) {\n" % (depth))
    for k, v in trie.items():
        f.write("%scase %s: " % (ws, repr(k)))
        if k == '\0':
            f.write("return %s;\n" % (v))
        else:
            write_trie(f, v, depth + 1)
    f.write("%s}" % ("  " * (depth + 1)))
    if depth != 0:
        f.write(" break;")
    f.write("\n")

with open(output_file, 'w') as f:
    f.write("#include <string.h>\n")
    f.write("#include \"trie.h\"\n\n")
    f.write("/* auto-generated by triegen.py */\n\n")
    f.write("int trie_get(const char* key) {\n  ")
    write_trie(f, trie, 0)
    f.write("  return -1;\n}\n")
            

