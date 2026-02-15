%{
#include "mpp.tab.h"
#include <string.h>
#include <stdlib.h>
%}

%option yylineno
%option noyywrap

DIGIT       [0-9]
ID_PATTERN  [a-zA-Z_][a-zA-Z0-9_]*
INT_PAT     {DIGIT}+
FLOAT_PAT   {DIGIT}+\.{DIGIT}+
CHAR_PAT    \'([^\\\n]|(\\.))\'
STR_PAT     \"([^\"\\]|\\.)*\"

%%

"start"         { return START; }
"finish"        { return FINISH; }
"when"          { return WHEN; }
"otherwise"     { return OTHERWISE; }
"repeat"        { return REPEAT; }
"loop"          { return LOOP; }
"doreturn"      { return DORETURN; }
"show"          { return SHOW; }
"take"          { return TAKE; }
"number"        { return TYPE_NUMBER; }
"decimal"       { return TYPE_DECIMAL; }
"letter"        { return TYPE_LETTER; }
"yes"           { return YES; }
"no"            { return NO; }
"nothing"       { return NOTHING; }
"stop"          { return STOP; }
"skip"          { return SKIP; }
"define"        { return DEFINE; }
"main"          { return MAIN; }

"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return MUL; }
"/"             { return DIV; }
"%"             { return MOD; }
"=="            { return EQ; }
"!="            { return NE; }
"<="            { return LE; }
">="            { return GE; }
"<"             { return LT; }
">"             { return GT; }
"&&"            { return AND; }
"||"            { return OR; }
"!"             { return NOT; }
"="             { return ASSIGN; }
"("             { return LPAREN; }
")"             { return RPAREN; }
"["             { return LBRACKET; }
"]"             { return RBRACKET; }
";"             { return SEMICOLON; }
","             { return COMMA; }

{FLOAT_PAT} {
    yylval.floatval = atof(yytext);
    return FLOAT_LITERAL;
}

{INT_PAT} {
    yylval.intval = atoi(yytext);
    return INTEGER_LITERAL;
}

{CHAR_PAT} {
    yylval.strval = strdup(yytext);
    return CHAR_LITERAL;
}

{STR_PAT} {
    yylval.strval = strdup(yytext);
    return STRING_LITERAL;
}

{ID_PATTERN} {
    yylval.strval = strdup(yytext);
    return IDENTIFIER;
}

[ \t\r\n]+      { /* ignore whitespace */ }
"//".* { /* ignore comment */ }
"/*"([^*]|\*+[^*/])*\*+"/" { /* ignore block comment */ }

. { printf("Invalid character '%s' at line %d\n", yytext, yylineno); }

%%