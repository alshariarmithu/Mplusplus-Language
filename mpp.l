%{
#include <stdio.h>
#include <string.h>

int token_count = 0;

void print_token(const char* token_type, const char* lexeme) {
    token_count++;
    printf("%-20s %-20s (Line: %d)\n", token_type, lexeme, yylineno);
}

%}

%option yylineno
%option noyywrap

DIGIT       [0-9]
LETTER      [a-zA-Z]
IDENTIFIER  [a-zA-Z_][a-zA-Z0-9_]*
INTEGER     {DIGIT}+
FLOAT       {DIGIT}+\.{DIGIT}+
CHAR        '.'
STRING      \"([^\"\\]|\\.)*\"

%%

"start"         { print_token("KEYWORD", yytext); }
"finish"        { print_token("KEYWORD", yytext); }
"when"          { print_token("KEYWORD", yytext); }
"otherwise"     { print_token("KEYWORD", yytext); }
"repeat"        { print_token("KEYWORD", yytext); }
"loop"          { print_token("KEYWORD", yytext); }
"doreturn"      { print_token("KEYWORD", yytext); }
"show"          { print_token("KEYWORD", yytext); }
"take"          { print_token("KEYWORD", yytext); }
"number"        { print_token("KEYWORD", yytext); }
"decimal"       { print_token("KEYWORD", yytext); }
"letter"        { print_token("KEYWORD", yytext); }
"yes"           { print_token("KEYWORD", yytext); }
"no"            { print_token("KEYWORD", yytext); }
"nothing"       { print_token("KEYWORD", yytext); }
"stop"          { print_token("KEYWORD", yytext); }
"skip"          { print_token("KEYWORD", yytext); }
"define"        { print_token("KEYWORD", yytext); }
"main"          { print_token("KEYWORD", yytext); }


"+"             { print_token("OPERATOR", yytext); }
"-"             { print_token("OPERATOR", yytext); }
"*"             { print_token("OPERATOR", yytext); }
"/"             { print_token("OPERATOR", yytext); }
"%"             { print_token("OPERATOR", yytext); }
"=="            { print_token("OPERATOR", yytext); }
"!="            { print_token("OPERATOR", yytext); }
"<="            { print_token("OPERATOR", yytext); }
">="            { print_token("OPERATOR", yytext); }
"="             { print_token("OPERATOR", yytext); }
"<"             { print_token("OPERATOR", yytext); }
">"             { print_token("OPERATOR", yytext); }
"&&"            { print_token("OPERATOR", yytext); }
"||"            { print_token("OPERATOR", yytext); }
"!"             { print_token("OPERATOR", yytext); }


"("             { print_token("DELIMITER", yytext); }
")"             { print_token("DELIMITER", yytext); }
"["             { print_token("DELIMITER", yytext); }
"]"             { print_token("DELIMITER", yytext); }
";"             { print_token("DELIMITER", yytext); }
","             { print_token("DELIMITER", yytext); }


{INTEGER}       { print_token("INTEGER_LITERAL", yytext); }
{FLOAT}         { print_token("FLOAT_LITERAL", yytext); }
{CHAR}          { print_token("CHAR_LITERAL", yytext); }
{STRING}        { print_token("STRING_LITERAL", yytext); }

{IDENTIFIER}    { print_token("IDENTIFIER", yytext); }

    /* Whitespace and Comments */
[ \t]+          { /* ignore whitespace */ }
\n              { /* ignore newlines, but yylineno tracks them */ }
"//".*          { /* single-line comment */ }
"/*"([^*]|\*+[^*/])*\*+"/"  { /* multi-line comment */ }
[\r\n]+         { }


.               { printf("ERROR: Invalid character '%s' at line %d\n", yytext, yylineno); }

%%

int main(int argc, char** argv) {
    FILE *file;
    
    if (argc > 1) {
        file = fopen(argv[1], "r");
        if (!file) {
            fprintf(stderr, "Error: Cannot open file '%s'\n", argv[1]);
            return 1;
        }
        yyin = file;
    } else {
        printf("Enter M++ code (Ctrl+D or Ctrl+Z to end):\n");
        yyin = stdin;
    }
    
    printf("\n%-20s %-20s %s\n", "TOKEN TYPE", "LEXEME", "LINE");
    printf("----------------------------------------------------\n");
    
    yylex();
    
    printf("----------------------------------------------------\n");
    printf("Total tokens: %d\n", token_count);
    
    if (argc > 1) {
        fclose(file);
    }
    
    return 0;
}