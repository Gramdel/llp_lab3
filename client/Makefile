GLIB = $(pkg-config --cflags --libs glib-2.0)

THRIFT = $(pkg-config --cflags --libs thrift)

all: main

clean:
	rm -rf parser.tab.c parser.tab.h parser.output lex.yy.c main debug

parser: parser.y
	bison -d parser.y

parser_debug: parser.y
	bison -d --verbose --debug parser.y

lexer: lexer.l
	flex lexer.l

main: parser lexer graphql_ast.c main.c
	gcc parser.tab.c lex.yy.c graphql_ast.c main.c $(GLIB) $(THRIFT) -o main

debug: parser_debug lexer graphql_ast.c main.c
	gcc parser.tab.c lex.yy.c graphql_ast.c main.c $(GLIB) $(THRIFT) -o debug

.PHONY: all clean parser parser_debug lexer
