all: build

build:
	mkdir -p ../compiler/bin
	yacc -o ./include/y.tab.c -d ./src/compiler.y
	lex -o ./include/lex.yy.c ./src/compiler.l
	gcc -g ./src/**.c ./src/compile/**.c ./include/**.c -o ./bin/compiler -lm

run: build
	@clear
	./bin/compiler -o ./test.s -d ./test_dbg.txt ./test/com_test/1.sil

compile:
	mips-linux-gnu-gcc-10 test.s 
	qemu-mips -L /usr/mips-linux-gnu/ ./a.out 
	echo $?

crun: build
	clear
	./bin/compiler

clean:
	rm -f ./**/**.yy.c ./**/**.tab.* ./bin/compiler

valgrind: build
	 valgrind --leak-check=full --show-leak-kinds=all ./bin/compiler -o ./test.txt -d ./test_dbg.txt ./test/com_test/1.sil