all: main

parser: parser.y
	bison -d parser.y

lexer: lexer.l
	flex lexer.l

main: parser lexer
	gcc parser.tab.c lex.yy.c -o main

.PHONY: all
