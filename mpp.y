%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void yyerror(const char *s);
int yylex(void);
extern int yylineno;
extern FILE *yyin;
FILE *output_file;

// --- AST Node Types ---
typedef enum { 
    NODE_STMT_LIST, NODE_ASSIGN, NODE_SHOW, NODE_IF, 
    NODE_LOOP, NODE_REPEAT, NODE_VAR, NODE_NUM, NODE_OP, 
    NODE_CALL, NODE_FUNC_DEF 
} NodeType;

typedef struct Node {
    NodeType type;
    int op;             
    char *id;           
    char *str;          
    double val;         
    struct Node *left, *right, *cond, *step, *body, *else_part, *next;
} Node;

// --- Tables ---
typedef struct { char *name; double val; } Symbol;
Symbol var_table[1000];
int var_count = 0;

typedef struct { char *name; Node *body; } Function;
Function func_table[100];
int func_count = 0;

// Variable Logic
void set_var(char *name, double val) {
    for(int i=0; i<var_count; i++) {
        if(strcmp(var_table[i].name, name) == 0) { var_table[i].val = val; return; }
    }
    var_table[var_count].name = strdup(name);
    var_table[var_count++].val = val;
}

double get_var(char *name) {
    for(int i=0; i<var_count; i++) {
        if(strcmp(var_table[i].name, name) == 0) return var_table[i].val;
    }
    return 0;
}

// Function Logic
void register_func(char *name, Node *body) {
    func_table[func_count].name = strdup(name);
    func_table[func_count++].body = body;
}

Node* find_func(char *name) {
    for(int i=0; i<func_count; i++) {
        if(strcmp(func_table[i].name, name) == 0) return func_table[i].body;
    }
    return NULL;
}

// --- Interpreter Core ---
Node* create_node(NodeType type) {
    Node *n = calloc(1, sizeof(Node));
    n->type = type;
    return n;
}

double execute(Node *n); // Forward declaration

double eval(Node *n) {
    if (!n) return 0;
    if (n->type == NODE_NUM) return n->val;
    if (n->type == NODE_VAR) return get_var(n->id);
    if (n->type == NODE_CALL) return execute(find_func(n->id));
    if (n->type == NODE_OP) {
        double l = eval(n->left);
        double r = eval(n->right);
        switch(n->op) {
            case '+': return l + r; case '-': return l - r;
            case '*': return l * r; case '/': return (r != 0) ? l / r : 0;
            case '>': return l > r; case '<': return l < r;
            case 'E': return l == r;
        }
    }
    return 0;
}

double execute(Node *n) {
    if (!n) return 0;
    switch(n->type) {
        case NODE_STMT_LIST: execute(n->left); execute(n->next); break;
        case NODE_ASSIGN: set_var(n->id, eval(n->left)); break;
        case NODE_SHOW:
            if (n->str) {
                char *s = strdup(n->str); s[strlen(s)-1] = '\0';
                fprintf(output_file, "%s\n", s+1); free(s);
            } else fprintf(output_file, "%g\n", eval(n->left));
            break;
        case NODE_IF:
            if (eval(n->cond)) execute(n->body);
            else if (n->else_part) execute(n->else_part);
            break;
        case NODE_LOOP:
            execute(n->left); // init
            while (eval(n->cond)) { execute(n->body); execute(n->step); }
            break;
        case NODE_REPEAT:
            while (eval(n->cond)) { execute(n->body); } // "While-style" repeat
            break;
        case NODE_CALL:
            return execute(find_func(n->id));
    }
    return 0;
}
%}

%union {
    double dval;
    char *strval;
    struct Node *node;
}

%token <dval> INTEGER_LITERAL FLOAT_LITERAL
%token <strval> IDENTIFIER STRING_LITERAL
%token START FINISH WHEN OTHERWISE REPEAT LOOP SHOW TYPE_NUMBER TYPE_DECIMAL DEFINE ASSIGN SEMICOLON LPAREN RPAREN PLUS MINUS MUL DIV GT LT EQ MAIN

%type <node> expr statement statements block declaration assignment show_stmt if_stmt loop_stmt repeat_stmt

%left EQ GT LT
%left PLUS MINUS
%left MUL DIV

%%

program: elements { 
    Node *main_node = find_func("main");
    if(main_node) execute(main_node);
    else fprintf(stderr, "Error: main() function not defined.\n");
};

elements: /* empty */ | elements element ;

element: 
    DEFINE type_spec IDENTIFIER LPAREN RPAREN block { register_func($3, $6); }
    | DEFINE type_spec MAIN LPAREN RPAREN block { register_func("main", $6); }
;

type_spec: TYPE_NUMBER | TYPE_DECIMAL ;

block: START statements FINISH { $$ = $2; } ;

statements: 
    statement { $$ = $1; }
    | statements statement { 
        Node *n = create_node(NODE_STMT_LIST); 
        n->left = $1; n->next = $2; $$ = n; 
    }
;

statement:
      declaration SEMICOLON | assignment SEMICOLON | show_stmt SEMICOLON
    | if_stmt | loop_stmt | repeat_stmt | block
    | IDENTIFIER LPAREN RPAREN SEMICOLON { $$ = create_node(NODE_CALL); $$->id = $1; }
;

declaration: type_spec IDENTIFIER ASSIGN expr { 
    $$ = create_node(NODE_ASSIGN); $$->id = $2; $$->left = $4; 
};

assignment: IDENTIFIER ASSIGN expr { 
    $$ = create_node(NODE_ASSIGN); $$->id = $1; $$->left = $3; 
};

show_stmt: 
    SHOW LPAREN expr RPAREN { $$ = create_node(NODE_SHOW); $$->left = $3; }
    | SHOW LPAREN STRING_LITERAL RPAREN { $$ = create_node(NODE_SHOW); $$->str = $3; }
;

if_stmt: 
    WHEN LPAREN expr RPAREN block { $$ = create_node(NODE_IF); $$->cond = $3; $$->body = $5; }
    | WHEN LPAREN expr RPAREN block OTHERWISE block {
        $$ = create_node(NODE_IF); $$->cond = $3; $$->body = $5; $$->else_part = $7;
    }
;

loop_stmt: LOOP LPAREN declaration SEMICOLON expr SEMICOLON assignment RPAREN block {
    $$ = create_node(NODE_LOOP); $$->left = $3; $$->cond = $5; $$->step = $7; $$->body = $9;
};

repeat_stmt: REPEAT LPAREN expr RPAREN block {
    $$ = create_node(NODE_REPEAT); $$->cond = $3; $$->body = $5;
};

expr:
    INTEGER_LITERAL { $$ = create_node(NODE_NUM); $$->val = $1; }
    | IDENTIFIER    { $$ = create_node(NODE_VAR); $$->id = $1; }
    | IDENTIFIER LPAREN RPAREN { $$ = create_node(NODE_CALL); $$->id = $1; }
    | expr PLUS expr  { $$ = create_node(NODE_OP); $$->op = '+'; $$->left = $1; $$->right = $3; }
    | expr MINUS expr { $$ = create_node(NODE_OP); $$->op = '-'; $$->left = $1; $$->right = $3; }
    | expr MUL expr   { $$ = create_node(NODE_OP); $$->op = '*'; $$->left = $1; $$->right = $3; }
    | expr DIV expr   { $$ = create_node(NODE_OP); $$->op = '/'; $$->left = $1; $$->right = $3; }
    | expr GT expr    { $$ = create_node(NODE_OP); $$->op = '>'; $$->left = $1; $$->right = $3; }
    | expr LT expr    { $$ = create_node(NODE_OP); $$->op = '<'; $$->left = $1; $$->right = $3; }
    | expr EQ expr    { $$ = create_node(NODE_OP); $$->op = 'E'; $$->left = $1; $$->right = $3; }
    | LPAREN expr RPAREN { $$ = $2; }
;

%%

void yyerror(const char *s) { fprintf(stderr, "Line %d: %s\n", yylineno, s); }

int main() {
    yyin = fopen("input.mpp", "r");
    output_file = fopen("output.txt", "w");
    yyparse();
    return 0;
}