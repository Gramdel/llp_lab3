%option yylineno noyywrap nounput noinput

%{
#include <stdio.h>
#include <stdbool.h>
#include "parser.tab.h"
%}

%%
\(                             { return L_PARENTHESIS; }
\)                             { return R_PARENTHESIS; }
\{                             { return L_BRACE; }
\}                             { return R_BRACE; }
\[                             { return L_BRACKET; }
\]                             { return R_BRACKET; }
,                              { return COMMA; }
:                              { return COLON; }
select                         { return SELECT; }
insert                         { return INSERT; }
update                         { return UPDATE; }
delete                         { return DELETE; }
values                         { return VALUES; }
filter                         { return FILTER; }
eq                             { return COMPARE_OP; }
neq                            { return COMPARE_OP; }
gt                             { return COMPARE_OP; }
gte                            { return COMPARE_OP; }
le                             { return COMPARE_OP; }
lee                            { return COMPARE_OP; }
and                            { return LOGICAL_BOP; }
or                             { return LOGICAL_BOP; }
not                            { return LOGICAL_UOP; }
like                           { return LIKE; }
true                           { yylval.bool_val = true; return TRUE; }
false                          { yylval.bool_val = false; return FALSE; }
[+-]?[0-9]+                    { yylval.int_val = atoi(yytext); return INT; }
[+-]?[0-9]\.?[0-9]*            { yylval.double_val = atof(yytext); return DOUBLE; }
\"[^\"]*\"                     { yylval.str = strdup(yytext); return STRING; }
[A-Za-z_][A-Za-z0-9_]{0,11}    { yylval.str = strdup(yytext); return NAME; }
.                              { /* Игнорируем всё остальное */ }
%%
