%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void yyerror(const char *s);
int yylex(void);
extern int yylineno;
%}

%union {
    int intval;
    float floatval;
    char *strval;
}

%token <intval> INTEGER_LITERAL
%token <floatval> FLOAT_LITERAL
%token <strval> IDENTIFIER CHAR_LITERAL STRING_LITERAL
%token START FINISH WHEN OTHERWISE REPEAT LOOP DORETURN SHOW TAKE
%token TYPE_NUMBER TYPE_DECIMAL TYPE_LETTER YES NO NOTHING STOP SKIP DEFINE MAIN
%token EQ NE LE GE LT GT AND OR NOT LBRACKET RBRACKET COMMA
%token PLUS MINUS MUL DIV MOD ASSIGN LPAREN RPAREN SEMICOLON

%left OR
%left AND
%left EQ NE
%left LT GT LE GE
%left PLUS MINUS
%left MUL DIV MOD
%right NOT

%%

program:
      elements
;

elements:
      /* empty */
    | elements element
;

element:
      statement
    | function_def
;

function_def:
    DEFINE type_spec IDENTIFIER LPAREN parameters RPAREN block
;

type_spec:
      TYPE_NUMBER
    | TYPE_DECIMAL
    | TYPE_LETTER
    | NOTHING
;

parameters:
      /* empty */
    | parameter_list
;

parameter_list:
      parameter
    | parameter_list COMMA parameter
;

parameter:
    type_spec IDENTIFIER
;

statements:
      /* empty */
    | statements statement
;

statement:
      declaration SEMICOLON
    | assignment SEMICOLON
    | show_stmt SEMICOLON
    | take_stmt SEMICOLON
    | skip_stmt SEMICOLON
    | stop_stmt SEMICOLON
    | return_stmt SEMICOLON
    | if_stmt
    | loop_stmt
    | repeat_stmt
    | block
;

block:
    START statements FINISH
;

declaration:
      type_spec IDENTIFIER ASSIGN expr
    | type_spec IDENTIFIER
    | type_spec IDENTIFIER LBRACKET INTEGER_LITERAL RBRACKET
;

assignment:
      IDENTIFIER ASSIGN expr
    | IDENTIFIER LBRACKET expr RBRACKET ASSIGN expr
;

show_stmt:
    SHOW LPAREN expr RPAREN
;

take_stmt:
    TAKE IDENTIFIER
;

skip_stmt:
    SKIP
;

stop_stmt:
    STOP
;

return_stmt:
    DORETURN expr
;

if_stmt:
      WHEN LPAREN expr RPAREN block
    | WHEN LPAREN expr RPAREN block OTHERWISE block
;

loop_stmt:
    LOOP LPAREN declaration SEMICOLON expr SEMICOLON assignment RPAREN block
;

repeat_stmt:
    REPEAT LPAREN expr RPAREN block
;

expr:
      INTEGER_LITERAL
    | FLOAT_LITERAL
    | IDENTIFIER
    | CHAR_LITERAL
    | STRING_LITERAL
    | YES
    | NO
    | IDENTIFIER LBRACKET expr RBRACKET
    | IDENTIFIER LPAREN arguments RPAREN
    | expr PLUS expr
    | expr MINUS expr
    | expr MUL expr
    | expr DIV expr
    | expr MOD expr
    | expr EQ expr
    | expr NE expr
    | expr LT expr
    | expr GT expr
    | expr LE expr
    | expr GE expr
    | expr AND expr
    | expr OR expr
    | NOT expr
    | LPAREN expr RPAREN
;

arguments:
      /* empty */
    | arg_list
;

arg_list:
      expr
    | arg_list COMMA expr
;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Syntax Error at line %d: %s\n", yylineno, s);
}

int main(void) {
    printf("Parsing M++ program...\n");
    if (yyparse() == 0) {
        printf("Parsing completed successfully!\n");
    } else {
        printf("Parsing failed.\n");
    }
    return 0;
}