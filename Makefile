make: trie $(wildcard src/*.c)
	mkdir -p bin
	gcc src/*.c -Wall -Wextra -pedantic -ggdb -o bin/sysh

trie: gen/triegen.py gen/commands
	python gen/triegen.py gen/commands gen/syscalls_x86_64 src/trie.c

clean: 
	rm -f src/trie.c
	rm -rf bin

run: make
	./bin/sysh
