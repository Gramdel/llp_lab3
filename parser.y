%{
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "graphql_ast.h"

extern int yylex();
extern int yylineno;

void yyerror(const char *msg) {
    fprintf(stderr, "Error on line %d: %s\n", yylineno, msg);
}
%}

%union {
    bool boolVal;
    int intVal;
    double doubleVal;
    const char* strVal;
    astNode* node;
    nodeType opType;
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
%token<opType> COMPARE_OP
%token<opType> LOGICAL_BOP
%token<opType> LOGICAL_UOP
%token<boolVal> TRUE
%token<boolVal> FALSE
%token<intVal> INT
%token<doubleVal> DOUBLE
%token<strVal> STRING
%token<strVal> NAME

%type<node> select select_next insert_or_select_next insert_next update_next select_object mutate_object values element key value filter operation compare_op logical_op
%type<strVal> schema_name

%%
init: query

query: select { printNode($1, 0); } | insert | update | delete

select: SELECT L_BRACE select_next R_BRACE { $$ = newQueryNode(SELECT_QUERY, NULL, $3); }

insert: INSERT L_BRACE insert_or_select_next R_BRACE

update: UPDATE L_BRACE update_next R_BRACE

delete: DELETE L_BRACE select_next R_BRACE

select_next: select_object { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY, $1, NULL), NULL); }
           | select_object COMMA select_next { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY, $1, NULL), $3); }
           | select_object L_BRACE select_next R_BRACE { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY, $1, $3), NULL); }
           | select_object L_BRACE select_next R_BRACE COMMA select_next { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY, $1, $3), $6); }

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

select_object: schema_name { $$ = newObjectNode($1, NULL, NULL); }
             | schema_name L_PARENTHESIS filter R_PARENTHESIS { $$ = newObjectNode($1, NULL, $3); }

mutate_object: schema_name L_PARENTHESIS values R_PARENTHESIS { $$ = newObjectNode($1, $3, NULL); }
             | schema_name L_PARENTHESIS values COMMA filter R_PARENTHESIS { $$ = newObjectNode($1, $3, $5); }

schema_name: NAME { $$ = $1; }

values: VALUES COLON L_BRACKET element R_BRACKET { $$ = newValuesNode($4); }

element: L_BRACE key COLON value R_BRACE { $$ = newElementSetNode(newElementNode($2, $4)); }
       | element COMMA L_BRACE key COLON value R_BRACE { addNextElementToSet($1, newElementSetNode(newElementNode($4, $6))); $$ = $1; }

key: NAME { $$ = newStrValNode($1); }

value: TRUE { $$ = newBoolValNode($1); }
     | FALSE { $$ = newBoolValNode($1); }
     | INT { $$ = newIntValNode($1); }
     | DOUBLE { $$ = newDoubleValNode($1); }
     | STRING { $$ = newStrValNode($1); }

filter: FILTER COLON operation { $$ = newFilterNode($3); }

operation: compare_op { $$ = $1; }
         | logical_op { $$ = $1; }

compare_op: COMPARE_OP L_PARENTHESIS key COMMA value R_PARENTHESIS { $$ = newOperationNode($1, $3, $5); }

logical_op: LOGICAL_UOP L_PARENTHESIS operation R_PARENTHESIS { $$ = newOperationNode($1, $3, NULL); }
          | LOGICAL_BOP L_PARENTHESIS operation COMMA operation R_PARENTHESIS { $$ = newOperationNode($1, $3, $5); }

%%
