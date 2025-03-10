all: build

build:
	mkdir -p ../compiler/bin
	yacc -o ./include/y.tab.c -d ./src/compiler.y
	lex -o ./include/lex.yy.c ./src/compiler.l
	gcc -g ./src/**.c ./include/**.c -o ./bin/compiler -lm

run: build
	@clear
	./bin/compiler < ./test/5.sil

crun: build
	clear
	./bin/compiler

clean:
	rm -f ./**/**.yy.c ./**/**.tab.* ./bin/compiler

valgrind: build
	 valgrind --leak-check=full --show-leak-kinds=all ./bin/compiler < ./test/3.prog