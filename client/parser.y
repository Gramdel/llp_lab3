%{
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "graphql_ast.h"

#define MAX_NAME_LENGTH 12

extern int yylex();
extern int yylineno;

void yyerror(astNode** tree, const char *msg) {
    fprintf(stderr, "Error on line %d: %s\n", yylineno, msg);
}
%}

%parse-param {astNode** tree}

%union {
    bool boolVal;
    int intVal;
    double doubleVal;
    char* strVal;
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
%token<opType> LIKE_OP
%token<opType> LOGICAL_BOP
%token<opType> LOGICAL_UOP
%token<boolVal> BOOL
%token<intVal> INT
%token<doubleVal> DOUBLE
%token<strVal> STRING
%token<strVal> NAME
%token EXIT

%type<node> query
%type<node> select
%type<node> insert
%type<node> update
%type<node> delete
%type<node> select_next
%type<node> insert_or_select_next
%type<node> insert_next
%type<node> update_next
%type<node> select_object
%type<node> mutate_object
%type<node> values
%type<node> element
%type<node> key
%type<node> value
%type<node> filter
%type<node> operation
%type<node> compare_op
%type<node> like_op
%type<node> logical_op
%type<strVal> schema_name

%left COMMA

%%
init: query { *tree = $1; YYACCEPT; }
    | EXIT { *tree = NULL; YYACCEPT; }

query: select { $$ = $1; }
     | insert { $$ = $1; }
     | update { $$ = $1; }
     | delete { $$ = $1; }

select: SELECT L_BRACE select_next R_BRACE { $$ = newQueryNode(SELECT_QUERY_NODE, NULL, $3); }

insert: INSERT L_BRACE insert_or_select_next R_BRACE { $$ = newQueryNode(INSERT_QUERY_NODE, NULL, $3); }

update: UPDATE L_BRACE update_next R_BRACE { $$ = newQueryNode(UPDATE_QUERY_NODE, NULL, $3); }

delete: DELETE L_BRACE select_next R_BRACE { $$ = newQueryNode(DELETE_QUERY_NODE, NULL, $3); }

select_next: select_object { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY_NODE, $1, NULL)); }
           | select_object L_BRACE select_next R_BRACE { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY_NODE, $1, $3)); }
           | select_next COMMA select_next { addNextQueryToSet($1, $3); $$ = $1; }

insert_or_select_next: insert_next %prec COMMA { $$ = $1; }
                     | select_object L_BRACE insert_or_select_next R_BRACE { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY_NODE, $1, $3)); }
                     | insert_or_select_next COMMA insert_or_select_next { addNextQueryToSet($1, $3); $$ = $1; }

insert_next: mutate_object { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY_NODE, $1, NULL)); }
           | mutate_object L_BRACE insert_next R_BRACE { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY_NODE, $1, $3)); }
           | insert_next COMMA insert_next { addNextQueryToSet($1, $3); $$ = $1; }

update_next: mutate_object { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY_NODE, $1, NULL)); }
           | mutate_object L_BRACE update_next R_BRACE { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY_NODE, $1, $3)); }
           | select_object L_BRACE update_next R_BRACE { $$ = newQuerySetNode(newQueryNode(NESTED_QUERY_NODE, $1, $3)); }
           | update_next COMMA update_next { addNextQueryToSet($1, $3); $$ = $1; }

select_object: schema_name { $$ = newObjectNode($1, NULL, NULL); }
             | schema_name L_PARENTHESIS filter R_PARENTHESIS { $$ = newObjectNode($1, NULL, $3); }

mutate_object: schema_name L_PARENTHESIS values R_PARENTHESIS { $$ = newObjectNode($1, $3, NULL); }
             | schema_name L_PARENTHESIS values COMMA filter R_PARENTHESIS { $$ = newObjectNode($1, $3, $5); }

schema_name: NAME { if(strlen($1) > MAX_NAME_LENGTH) { yyerror(tree, "name of schema is too long"); YYABORT; } $$ = $1; }

values: VALUES COLON L_BRACKET element R_BRACKET { $$ = newValuesNode($4); }

element: L_BRACE key COLON value R_BRACE { $$ = newElementSetNode(newElementNode($2, $4)); }
       | element COMMA element { addNextElementToSet($1, $3); $$ = $1; }

key: NAME { if(strlen($1) > MAX_NAME_LENGTH) { yyerror(tree, "key is too long"); YYABORT; } $$ = newKeyNode($1); }

value: BOOL { $$ = newBoolValNode($1); }
     | INT { $$ = newIntValNode($1); }
     | DOUBLE { $$ = newDoubleValNode($1); }
     | STRING { $$ = newStrValNode($1); }

filter: FILTER COLON operation { $$ = newFilterNode($3); }

operation: compare_op { $$ = $1; }
         | like_op { $$ = $1; }
         | logical_op { $$ = $1; }

compare_op: COMPARE_OP L_PARENTHESIS key COMMA value R_PARENTHESIS { $$ = newOperationNode($1, $3, $5); }
          | COMPARE_OP L_PARENTHESIS key COMMA key R_PARENTHESIS { $$ = newOperationNode($1, $3, $5); }

like_op: LIKE_OP L_PARENTHESIS key COMMA STRING R_PARENTHESIS { $$ = newOperationNode($1, $3, newStrValNode($5)); }
       | LIKE_OP L_PARENTHESIS key COMMA key R_PARENTHESIS { $$ = newOperationNode($1, $3, $5); }

logical_op: LOGICAL_UOP L_PARENTHESIS operation R_PARENTHESIS { $$ = newOperationNode($1, $3, NULL); }
          | LOGICAL_BOP L_PARENTHESIS operation COMMA operation R_PARENTHESIS { $$ = newOperationNode($1, $3, $5); }

%%
