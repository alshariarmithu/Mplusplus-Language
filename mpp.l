%{
/*=============================================================
  mpp.l  -  Lexer for the M++ language
  Fixes applied:
    - Added missing "stop" and "skip" keywords
    - Added "//" single-line comment support
    - Ensured FLOAT is matched before INTEGER to avoid partial matches
    - Handles all tokens declared in mpp.y
=============================================================*/
#include "mpp.tab.h"
#include <string.h>
#include <stdlib.h>
%}

%option noyywrap
%option yylineno

%%

"//"[^\n]*          { /* skip single-line comments */ }

"start"             { return START; }
"finish"            { return FINISH; }
"when"              { return WHEN; }
"otherwise"         { return OTHERWISE; }
"repeat"            { return REPEAT; }
"loop"              { return LOOP; }
"show"              { return SHOW; }
"take"              { return TAKE; }
"number"            { return TYPE_NUMBER; }
"decimal"           { return TYPE_DECIMAL; }
"letter"            { return TYPE_LETTER; }
"nothing"           { return NOTHING; }
"define"            { return DEFINE; }
"main"              { return MAIN; }
"doreturn"          { return DORETURN; }
"yes"               { return YES; }
"no"                { return NO; }
"stop"              { return STOP; }
"skip"              { return SKIP; }

"=="                { return EQ; }
"!="                { return NEQ; }
">="                { return GTE; }
"<="                { return LTE; }
">"                 { return GT; }
"<"                 { return LT; }
"+"                 { return PLUS; }
"-"                 { return MINUS; }
"*"                 { return MUL; }
"/"                 { return DIV; }
"="                 { return ASSIGN; }
"("                 { return LPAREN; }
")"                 { return RPAREN; }
"["                 { return LBRACK; }
"]"                 { return RBRACK; }
";"                 { return SEMICOLON; }
","                 { return COMMA; }

[0-9]+\.[0-9]+      { yylval.dval = atof(yytext); return FLOAT_LITERAL; }
[0-9]+              { yylval.dval = atof(yytext); return INTEGER_LITERAL; }
\"[^\"]*\"          { yylval.strval = strdup(yytext); return STRING_LITERAL; }
\'[^\']\'           { yylval.strval = strdup(yytext); return CHAR_LITERAL; }

[a-zA-Z_][a-zA-Z0-9_]*  { yylval.strval = strdup(yytext); return IDENTIFIER; }

[ \t\n\r]           { /* skip whitespace */ }
.                   { /* skip unknown characters */ }

%%
