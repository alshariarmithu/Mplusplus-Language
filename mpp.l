%{
/*=============================================================
  mpp.l  -  Lexer for the M++ language
  Features:
    - All keywords: start finish when otherwise repeat loop
      show take number decimal letter nothing define main
      doreturn yes no stop skip
    - Math built-in keywords: msin mcos mtan mlog msqrt mpow
    - Operators: == != >= <= > < + - * / =
    - Single-line comment support: //
    - Float matched before integer to avoid partial match
    - char literal stored via CHAR_LITERAL token
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

"msin"              { return MSIN; }
"mcos"              { return MCOS; }
"mtan"              { return MTAN; }
"mlog"              { return MLOG; }
"msqrt"             { return MSQRT; }
"mpow"              { return MPOW; }

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

[ \t\n\r]           { /* skip */ }
.                   { /* skip unknown characters */ }

%%
