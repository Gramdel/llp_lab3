THRIFT := `pkg-config --cflags --libs thrift_c_glib`

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
	gcc parser.tab.c lex.yy.c graphql_ast.c ../gen-c_glib/structs_types.c ../gen-c_glib/zgdb_service.c main.c $(THRIFT) -o main

debug: parser_debug lexer graphql_ast.c main.c
	gcc parser.tab.c lex.yy.c graphql_ast.c ../gen-c_glib/structs_types.c ../gen-c_glib/zgdb_service.c main.c $(THRIFT) -o debug

.PHONY: all clean parser parser_debug lexer
