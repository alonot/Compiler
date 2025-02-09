build:
	yacc -H -v ./src/compiler.y -o ./include/y.tab.c
	lex -o ./include/lex.yy.c ./src/compiler.l
	gcc -g ./src/**.c ./include/**.c -o ./bin/compiler.out -lm

run: build
	./bin/compiler.out

crun: build
	clear
	./bin/compiler.out

all: build
	mkdir -p ../compiler/bin

clean:
	rm -f ./**/**.yy.c ./**/**.tab.* ./bin/compiler.out