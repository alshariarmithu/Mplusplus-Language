%{
#include "mpp.tab.h"
%}
%option noyywrap
%option yylineno
%%
"start"     { return START; }
"finish"    { return FINISH; }
"when"      { return WHEN; }
"otherwise" { return OTHERWISE; }
"repeat"    { return REPEAT; }
"loop"      { return LOOP; }
"show"      { return SHOW; }
"number"    { return TYPE_NUMBER; }
"decimal"   { return TYPE_DECIMAL; }
"define"    { return DEFINE; }
"main"      { return MAIN; }
"=="        { return EQ; }
">"         { return GT; }
"<"         { return LT; }
"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MUL; }
"/"         { return DIV; }
"="         { return ASSIGN; }
"("         { return LPAREN; }
")"         { return RPAREN; }
";"         { return SEMICOLON; }
[0-9]+\.[0-9]+ { yylval.dval = atof(yytext); return FLOAT_LITERAL; }
[0-9]+      { yylval.dval = atof(yytext); return INTEGER_LITERAL; }
\"[^\"]*\"  { yylval.strval = strdup(yytext); return STRING_LITERAL; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.strval = strdup(yytext); return IDENTIFIER; }
[ \t\n\r]   ;
.           ;
%%