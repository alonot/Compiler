build:
	yacc -H -v ./src/compiler.y -o ./include/y.tab.c
	lex -o ./include/lex.yy.c ./src/compiler.l
	gcc -g ./src/**.c ./include/**.c -o ./bin/compiler -lm

run: build
	./bin/compiler

crun: build
	clear
	./bin/compiler

all: build
	mkdir -p ../compiler/bin

clean:
	rm -f ./**/**.yy.c ./**/**.tab.* ./bin/compiler