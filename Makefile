make: trie $(wildcard src/*.c)
	mkdir -p bin
	gcc src/*.c -Wall -Wextra -pedantic -ggdb -o bin/sysh

trie: triegen.py commands
	python triegen.py commands src/trie.c

clean: 
	rm -f src/trie.c
	rm -rf bin

run: make
	./bin/sysh
