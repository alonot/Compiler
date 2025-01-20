build:
	yacc -H calculator.y
	lex calculator.l
	gcc y.tab.c lex.yy.c -o calculator.out
run: build
	./calculator.out

crun: build
	clear
	./calculator.out

all: build
	mkdir -p ../compiler/bin

clean:
	rm -f ./*.yy.c ./*.tab.* ./calculator.out