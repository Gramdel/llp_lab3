%{
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

extern int yylex();
extern int yylineno;

void yyerror(const char *msg) {
    fprintf(stderr, "Error on line %d: %s\n", yylineno, msg);
}
%}

%union {
    bool bool_val;
    int int_val;
    double double_val;
    const char* str;
}

%token L_PARENTHESIS
%token R_PARENTHESIS
%token L_BRACE
%token R_BRACE
%token L_BRACKET
%token R_BRACKET
%token COMMA
%token COLON
%token SELECT
%token INSERT
%token UPDATE
%token DELETE
%token VALUES
%token FILTER
%token COMPARE_OP
%token LOGICAL_BOP
%token LOGICAL_UOP
%token LIKE
%token<bool_val> TRUE
%token<bool_val> FALSE
%token<int_val> INT
%token<double_val> DOUBLE
%token<str> STRING
%token<str> NAME

%%

query: select | insert | update | delete

select: SELECT L_BRACE select_next R_BRACE

insert: INSERT L_BRACE insert_or_select_next R_BRACE

update: UPDATE L_BRACE update_next R_BRACE

delete: DELETE L_BRACE select_next R_BRACE

select_next: select_object
           | select_object COMMA select_next
           | select_object L_BRACE select_next R_BRACE
           | select_object L_BRACE select_next R_BRACE COMMA select_next

insert_or_select_next: insert_next
                     | select_object L_BRACE insert_or_select_next R_BRACE
                     | select_object L_BRACE insert_or_select_next R_BRACE COMMA insert_or_select_next

insert_next: mutate_object
           | mutate_object COMMA insert_next
           | mutate_object L_BRACE insert_next R_BRACE
           | mutate_object L_BRACE insert_next R_BRACE COMMA insert_next

update_next: mutate_object
           | mutate_object COMMA update_next
           | mutate_object L_BRACE update_next R_BRACE
           | mutate_object L_BRACE update_next R_BRACE COMMA update_next
           | select_object L_BRACE update_next R_BRACE
           | select_object L_BRACE update_next R_BRACE COMMA update_next

select_object: schema_name
             | schema_name L_PARENTHESIS filter R_PARENTHESIS

mutate_object: schema_name L_PARENTHESIS values R_PARENTHESIS
             | schema_name L_PARENTHESIS values COMMA filter R_PARENTHESIS

schema_name: NAME

values: VALUES COLON L_BRACKET element R_BRACKET

element: L_BRACE key COLON value R_BRACE
       | element COMMA L_BRACE key COLON value R_BRACE

key: NAME

value: TRUE | FALSE | INT | DOUBLE | STRING

filter: FILTER COLON operation

operation: compare_op | logical_op

compare_op: COMPARE_OP L_PARENTHESIS key COMMA value R_PARENTHESIS

logical_op: LOGICAL_UOP L_PARENTHESIS operation R_PARENTHESIS
          | LOGICAL_BOP L_PARENTHESIS operation COMMA operation R_PARENTHESIS

%%
