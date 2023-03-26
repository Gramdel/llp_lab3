%option yylineno noyywrap nounput noinput

%{
#include <stdio.h>
#include <stdbool.h>
#include "graphql_ast.h"
#include "parser.tab.h"
%}

%%
\(                        { return L_PARENTHESIS; }
\)                        { return R_PARENTHESIS; }
\{                        { return L_BRACE; }
\}                        { return R_BRACE; }
\[                        { return L_BRACKET; }
\]                        { return R_BRACKET; }
,                         { return COMMA; }
:                         { return COLON; }
select                    { return SELECT; }
insert                    { return INSERT; }
update                    { return UPDATE; }
delete                    { return DELETE; }
values                    { return VALUES; }
filter                    { return FILTER; }
eq                        { yylval.opType = OP_EQ_NODE; return COMPARE_OP; }
neq                       { yylval.opType = OP_NEQ_NODE; return COMPARE_OP; }
gt                        { yylval.opType = OP_GT_NODE; return COMPARE_OP; }
gte                       { yylval.opType = OP_GTE_NODE; return COMPARE_OP; }
le                        { yylval.opType = OP_LE_NODE; return COMPARE_OP; }
lee                       { yylval.opType = OP_LEE_NODE; return COMPARE_OP; }
like                      { yylval.opType = OP_LIKE_NODE; return LIKE_OP; }
and                       { yylval.opType = OP_AND_NODE; return LOGICAL_BOP; }
or                        { yylval.opType = OP_OR_NODE; return LOGICAL_BOP; }
not                       { yylval.opType = OP_NOT_NODE; return LOGICAL_UOP; }
true                      { yylval.boolVal = true; return BOOL; }
false                     { yylval.boolVal = false; return BOOL; }
[+-]?[0-9]+               { yylval.intVal = atoi(yytext); return INT; }
[+-]?[0-9]*\.[0-9]*       { yylval.doubleVal = atof(yytext); return DOUBLE; }
\"[^\"]*\"                { yylval.strVal = strdup(yytext); return STRING; }
[A-Za-z_][A-Za-z0-9_]*    { yylval.strVal = strdup(yytext); return NAME; }
\n                        { /* Считываем перенос строки, чтобы работал yylineno */ }
.                         { /* Игнорируем всё остальное */ }
%%
