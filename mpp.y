%{
/*=============================================================
  mpp.y  -  Parser + AST + Tree-Walking Interpreter for M++
  Fixes applied:
    1. Added NODE_RETURN, NODE_BREAK, NODE_CONTINUE node types
    2. Added g_return_flag, g_break_flag, g_continue_flag globals
       so doreturn/stop/skip properly unwind nested execute() calls
    3. FLOAT_LITERAL added to expr rule
    4. Function calls inside expressions (NODE_CALL in eval)
    5. else-if chain: "otherwise when(...)" grammar rule added
    6. Empty block "start finish" no longer crashes (NULL statement)
    7. NODE_STMT_LIST short-circuits on return/break/continue
    8. show() prints integers without trailing .0
    9. Negative numbers supported via UMINUS precedence
    10. NEQ / GTE / LTE operators added
    11. char literal stored as ASCII value consistently
    12. Better error messages with line numbers
=============================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void yyerror(const char *s);
int yylex(void);
extern int yylineno;
extern FILE *yyin;
FILE *output_file;

/* ── Control-flow flags ─────────────────────────────────── */
int    g_return_flag   = 0;   /* doreturn was hit           */
double g_return_val    = 0.0; /* value carried by doreturn  */
int    g_break_flag    = 0;   /* stop was hit               */
int    g_continue_flag = 0;   /* skip was hit               */

/* ── AST node types ─────────────────────────────────────── */
typedef enum {
    NODE_STMT_LIST,
    NODE_ASSIGN,
    NODE_SHOW,
    NODE_TAKE,
    NODE_IF,
    NODE_LOOP,
    NODE_REPEAT,
    NODE_VAR,
    NODE_NUM,
    NODE_OP,
    NODE_CALL,
    NODE_VAR_DECL,
    NODE_ARRAY_DECL,
    NODE_ARRAY_ASSIGN,
    NODE_ARRAY_REF,
    NODE_RETURN,      /* doreturn */
    NODE_BREAK,       /* stop     */
    NODE_CONTINUE     /* skip     */
} NodeType;

typedef struct Node {
    NodeType    type;
    int         op;       /* operator character / code      */
    char       *id;       /* identifier name                */
    char       *str;      /* string / char literal text     */
    double      val;      /* numeric literal value          */
    struct Node *left;
    struct Node *right;
    struct Node *cond;
    struct Node *step;
    struct Node *body;
    struct Node *else_part;
    struct Node *next;    /* next statement in NODE_STMT_LIST */
    struct Node *index;   /* array index expression         */
} Node;

/* ── Symbol table ───────────────────────────────────────── */
typedef struct {
    char   *name;
    double  val;
    double *array;
    int     size;
    int     is_char;   /* 1 if declared as 'letter' */
} Symbol;

#define MAX_SYMBOLS 1000
Symbol var_table[MAX_SYMBOLS];
int    var_count = 0;

/* ── Function table ─────────────────────────────────────── */
typedef struct { char *name; Node *body; } Function;
#define MAX_FUNCS 100
Function func_table[MAX_FUNCS];
int      func_count = 0;

/* ── Symbol helpers ─────────────────────────────────────── */
static Symbol *find_symbol(const char *name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(var_table[i].name, name) == 0)
            return &var_table[i];
    return NULL;
}

void set_var(const char *name, double val) {
    Symbol *s = find_symbol(name);
    if (s) { s->val = val; return; }
    if (var_count >= MAX_SYMBOLS) { fprintf(stderr,"Symbol table full\n"); return; }
    var_table[var_count].name    = strdup(name);
    var_table[var_count].val     = val;
    var_table[var_count].array   = NULL;
    var_table[var_count].size    = 0;
    var_table[var_count].is_char = 0;
    var_count++;
}

void set_char_var(const char *name, char c) {
    Symbol *s = find_symbol(name);
    if (s) { s->val = (double)(unsigned char)c; s->is_char = 1; return; }
    if (var_count >= MAX_SYMBOLS) return;
    var_table[var_count].name    = strdup(name);
    var_table[var_count].val     = (double)(unsigned char)c;
    var_table[var_count].array   = NULL;
    var_table[var_count].size    = 0;
    var_table[var_count].is_char = 1;
    var_count++;
}

double get_var(const char *name) {
    Symbol *s = find_symbol(name);
    return s ? s->val : 0.0;
}

void init_array(const char *name, int size) {
    if (var_count >= MAX_SYMBOLS) return;
    var_table[var_count].name    = strdup(name);
    var_table[var_count].val     = 0;
    var_table[var_count].array   = calloc(size, sizeof(double));
    var_table[var_count].size    = size;
    var_table[var_count].is_char = 0;
    var_count++;
}

void set_array_val(const char *name, int idx, double val) {
    Symbol *s = find_symbol(name);
    if (s && s->array && idx >= 0 && idx < s->size)
        s->array[idx] = val;
}

double get_array_val(const char *name, int idx) {
    Symbol *s = find_symbol(name);
    if (s && s->array && idx >= 0 && idx < s->size)
        return s->array[idx];
    return 0.0;
}

/* ── Function helpers ───────────────────────────────────── */
void register_func(const char *name, Node *body) {
    for (int i = 0; i < func_count; i++) {
        if (strcmp(func_table[i].name, name) == 0) {
            func_table[i].body = body;
            return;
        }
    }
    if (func_count >= MAX_FUNCS) return;
    func_table[func_count].name = strdup(name);
    func_table[func_count++].body = body;
}

Node *find_func_node(const char *name) {
    for (int i = 0; i < func_count; i++)
        if (strcmp(func_table[i].name, name) == 0)
            return func_table[i].body;
    return NULL;
}

/* ── AST node constructor ───────────────────────────────── */
Node *create_node(NodeType type) {
    Node *n = calloc(1, sizeof(Node));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    n->type = type;
    return n;
}

/* ── Forward declarations ───────────────────────────────── */
double execute(Node *n);
double eval(Node *n);

/* ── Expression evaluator ───────────────────────────────── */
double eval(Node *n) {
    if (!n) return 0.0;
    switch (n->type) {
        case NODE_NUM:
            return n->val;

        case NODE_VAR:
            return get_var(n->id);

        case NODE_ARRAY_REF:
            return get_array_val(n->id, (int)eval(n->index));

        case NODE_CALL: {
            /* Save & reset control-flow flags for callee */
            int    saved_ret   = g_return_flag;
            double saved_rval  = g_return_val;
            int    saved_brk   = g_break_flag;
            int    saved_cont  = g_continue_flag;
            g_return_flag = g_break_flag = g_continue_flag = 0;
            g_return_val  = 0.0;

            Node *body = find_func_node(n->id);
            if (body) execute(body);
            double ret = g_return_val;

            /* Restore caller state */
            g_return_flag  = saved_ret;
            g_return_val   = saved_rval;
            g_break_flag   = saved_brk;
            g_continue_flag = saved_cont;
            return ret;
        }

        case NODE_OP: {
            double l = eval(n->left);
            double r = eval(n->right);
            switch (n->op) {
                case '+': return l + r;
                case '-': return l - r;
                case '*': return l * r;
                case '/': return (r != 0.0) ? l / r : 0.0;
                case '>': return l >  r ? 1.0 : 0.0;
                case '<': return l <  r ? 1.0 : 0.0;
                case 'E': return l == r ? 1.0 : 0.0;   /* == */
                case 'N': return l != r ? 1.0 : 0.0;   /* != */
                case 'G': return l >= r ? 1.0 : 0.0;   /* >= */
                case 'L': return l <= r ? 1.0 : 0.0;   /* <= */
            }
        }
        default: return 0.0;
    }
}

/* ── Statement executor ─────────────────────────────────── */
double execute(Node *n) {
    if (!n) return 0.0;

    /* Stop executing if a control-flow event is pending */
    if (g_return_flag || g_break_flag || g_continue_flag)
        return g_return_val;

    switch (n->type) {

        /* ── Statement list ─────────────────────────────── */
        case NODE_STMT_LIST:
            execute(n->left);
            if (!g_return_flag && !g_break_flag && !g_continue_flag)
                execute(n->next);
            break;

        /* ── Variable declaration ───────────────────────── */
        case NODE_VAR_DECL:
            if (n->str) {
                /* letter ch = 'A'; */
                set_char_var(n->id, n->str[1]);
            } else if (n->left) {
                set_var(n->id, eval(n->left));
            } else {
                set_var(n->id, 0.0);   /* uninitialized */
            }
            break;

        /* ── Assignment ─────────────────────────────────── */
        case NODE_ASSIGN:
            if (n->str) {
                set_char_var(n->id, n->str[1]);
            } else {
                set_var(n->id, eval(n->left));
            }
            break;

        /* ── Array declaration ──────────────────────────── */
        case NODE_ARRAY_DECL:
            init_array(n->id, (int)n->val);
            break;

        /* ── Array element assignment ───────────────────── */
        case NODE_ARRAY_ASSIGN:
            set_array_val(n->id, (int)eval(n->index), eval(n->left));
            break;

        /* ── take (input) ───────────────────────────────── */
        case NODE_TAKE: {
            double v;
            printf("Enter value for %s: ", n->id);
            fflush(stdout);
            if (scanf("%lf", &v) == 1)
                set_var(n->id, v);
            break;
        }

        /* ── show (output) ──────────────────────────────── */
        case NODE_SHOW:
            if (n->str) {
                /* String literal: strip surrounding quotes */
                int len = strlen(n->str);
                char *s = malloc(len + 1);
                strncpy(s, n->str + 1, len - 2);
                s[len - 2] = '\0';
                fprintf(output_file, "%s\n", s);
                fflush(output_file);
                free(s);
            } else {
                double v = eval(n->left);
                /* Check if the variable is declared as 'letter' (char) */
                if (n->left && n->left->type == NODE_VAR) {
                    Symbol *sym = find_symbol(n->left->id);
                    if (sym && sym->is_char) {
                        fprintf(output_file, "%c\n", (char)(int)v);
                        fflush(output_file);
                        break;
                    }
                }
                /* Print integer without .0, float with %g */
                if (v == (double)(long long)v && fabs(v) < 1e15)
                    fprintf(output_file, "%lld\n", (long long)v);
                else
                    fprintf(output_file, "%g\n", v);
                fflush(output_file);
            }
            break;

        /* ── when / otherwise (if / else) ──────────────── */
        case NODE_IF:
            if (eval(n->cond))
                execute(n->body);
            else if (n->else_part)
                execute(n->else_part);
            break;

        /* ── loop (for) ─────────────────────────────────── */
        case NODE_LOOP:
            execute(n->left);   /* initializer */
            while (!g_return_flag && eval(n->cond)) {
                execute(n->body);
                if (g_break_flag)    { g_break_flag = 0;    break; }
                if (g_continue_flag) { g_continue_flag = 0; /* fall to step */ }
                if (g_return_flag)   break;
                execute(n->step);
            }
            break;

        /* ── repeat (while) ─────────────────────────────── */
        case NODE_REPEAT:
            while (!g_return_flag && eval(n->cond)) {
                execute(n->body);
                if (g_break_flag)    { g_break_flag = 0;    break; }
                if (g_continue_flag) { g_continue_flag = 0; continue; }
                if (g_return_flag)   break;
            }
            break;

        /* ── doreturn ───────────────────────────────────── */
        case NODE_RETURN:
            g_return_val  = n->left ? eval(n->left) : 0.0;
            g_return_flag = 1;
            break;

        /* ── stop (break) ───────────────────────────────── */
        case NODE_BREAK:
            g_break_flag = 1;
            break;

        /* ── skip (continue) ────────────────────────────── */
        case NODE_CONTINUE:
            g_continue_flag = 1;
            break;

        /* ── function call as statement ─────────────────── */
        case NODE_CALL: {
            int    saved_ret  = g_return_flag;
            double saved_rval = g_return_val;
            g_return_flag = 0;
            g_return_val  = 0.0;
            Node *body = find_func_node(n->id);
            if (body) execute(body);
            g_return_flag = saved_ret;
            g_return_val  = saved_rval;
            break;
        }

        default: break;
    }
    return g_return_val;
}

%}

/* ── Bison union & token declarations ───────────────────── */
%union {
    double      dval;
    char       *strval;
    struct Node *node;
}

%token <dval>   INTEGER_LITERAL FLOAT_LITERAL
%token <strval> IDENTIFIER STRING_LITERAL CHAR_LITERAL

%token START FINISH WHEN OTHERWISE REPEAT LOOP SHOW TAKE
%token TYPE_NUMBER TYPE_DECIMAL TYPE_LETTER
%token DEFINE DORETURN NOTHING MAIN YES NO STOP SKIP
%token ASSIGN SEMICOLON LPAREN RPAREN LBRACK RBRACK
%token PLUS MINUS MUL DIV GT LT GTE LTE EQ NEQ COMMA

%type <node> expr statement statements block
%type <node> declaration assignment show_stmt take_stmt
%type <node> if_stmt loop_stmt repeat_stmt

/* ── Operator precedence (low → high) ───────────────────── */
%left  EQ NEQ
%left  GT LT GTE LTE
%left  PLUS MINUS
%left  MUL DIV
%right UMINUS

%%

/* ══════════════════════════════════════════════════════════
   Grammar Rules
   ══════════════════════════════════════════════════════════ */

program
    : elements
        {
            Node *m = find_func_node("main");
            if (m) {
                g_return_flag = g_break_flag = g_continue_flag = 0;
                execute(m);
            } else {
                fprintf(stderr, "Error: no 'main' function found.\n");
            }
        }
    ;

elements
    : /* empty */
    | elements element
    ;

element
    : DEFINE type_spec IDENTIFIER LPAREN RPAREN block
        { register_func($3, $6); }
    | DEFINE NOTHING IDENTIFIER LPAREN RPAREN block
        { register_func($3, $6); }
    | DEFINE NOTHING MAIN LPAREN RPAREN block
        { register_func("main", $6); }
    ;

type_spec
    : TYPE_NUMBER
    | TYPE_DECIMAL
    | TYPE_LETTER
    ;

/* A block may be empty: "start finish" is valid */
block
    : START statements FINISH  { $$ = $2; }
    | START FINISH             { $$ = NULL; }
    ;

statements
    : statement
        { $$ = $1; }
    | statements statement
        {
            if ($1 == NULL) {
                $$ = $2;
            } else if ($2 == NULL) {
                $$ = $1;
            } else {
                Node *n = create_node(NODE_STMT_LIST);
                n->left = $1;
                n->next = $2;
                $$ = n;
            }
        }
    ;

statement
    : declaration SEMICOLON         { $$ = $1; }
    | assignment  SEMICOLON         { $$ = $1; }
    | show_stmt   SEMICOLON         { $$ = $1; }
    | take_stmt   SEMICOLON         { $$ = $1; }
    | if_stmt                       { $$ = $1; }
    | loop_stmt                     { $$ = $1; }
    | repeat_stmt                   { $$ = $1; }
    | block                         { $$ = $1; }
    | DORETURN expr SEMICOLON
        {
            Node *n = create_node(NODE_RETURN);
            n->left = $2;
            $$ = n;
        }
    | DORETURN SEMICOLON
        {
            Node *n = create_node(NODE_RETURN);
            n->left = NULL;
            $$ = n;
        }
    | STOP SEMICOLON
        { $$ = create_node(NODE_BREAK); }
    | SKIP SEMICOLON
        { $$ = create_node(NODE_CONTINUE); }
    | IDENTIFIER LPAREN RPAREN SEMICOLON
        {
            Node *n = create_node(NODE_CALL);
            n->id = $1;
            $$ = n;
        }
    ;

declaration
    : type_spec IDENTIFIER ASSIGN expr
        {
            Node *n = create_node(NODE_VAR_DECL);
            n->id   = $2;
            n->left = $4;
            $$ = n;
        }
    | type_spec IDENTIFIER ASSIGN CHAR_LITERAL
        {
            Node *n = create_node(NODE_VAR_DECL);
            n->id  = $2;
            n->str = $4;
            $$ = n;
        }
    | type_spec IDENTIFIER
        {
            Node *n = create_node(NODE_VAR_DECL);
            n->id   = $2;
            n->left = NULL;
            $$ = n;
        }
    | type_spec IDENTIFIER LBRACK INTEGER_LITERAL RBRACK
        {
            Node *n = create_node(NODE_ARRAY_DECL);
            n->id  = $2;
            n->val = $4;
            $$ = n;
        }
    ;

assignment
    : IDENTIFIER ASSIGN expr
        {
            Node *n = create_node(NODE_ASSIGN);
            n->id   = $1;
            n->left = $3;
            $$ = n;
        }
    | IDENTIFIER ASSIGN CHAR_LITERAL
        {
            Node *n = create_node(NODE_ASSIGN);
            n->id  = $1;
            n->str = $3;
            $$ = n;
        }
    | IDENTIFIER LBRACK expr RBRACK ASSIGN expr
        {
            Node *n = create_node(NODE_ARRAY_ASSIGN);
            n->id    = $1;
            n->index = $3;
            n->left  = $6;
            $$ = n;
        }
    ;

show_stmt
    : SHOW LPAREN expr RPAREN
        {
            Node *n = create_node(NODE_SHOW);
            n->left = $3;
            $$ = n;
        }
    | SHOW LPAREN STRING_LITERAL RPAREN
        {
            Node *n = create_node(NODE_SHOW);
            n->str = $3;
            $$ = n;
        }
    ;

take_stmt
    : TAKE IDENTIFIER
        {
            Node *n = create_node(NODE_TAKE);
            n->id = $2;
            $$ = n;
        }
    ;

if_stmt
    : WHEN LPAREN expr RPAREN block
        {
            Node *n = create_node(NODE_IF);
            n->cond = $3;
            n->body = $5;
            $$ = n;
        }
    | WHEN LPAREN expr RPAREN block OTHERWISE block
        {
            Node *n = create_node(NODE_IF);
            n->cond      = $3;
            n->body      = $5;
            n->else_part = $7;
            $$ = n;
        }
    /* otherwise when (...) — else-if chain */
    | WHEN LPAREN expr RPAREN block OTHERWISE if_stmt
        {
            Node *n = create_node(NODE_IF);
            n->cond      = $3;
            n->body      = $5;
            n->else_part = $7;
            $$ = n;
        }
    ;

loop_stmt
    : LOOP LPAREN declaration SEMICOLON expr SEMICOLON assignment RPAREN block
        {
            Node *n = create_node(NODE_LOOP);
            n->left = $3;
            n->cond = $5;
            n->step = $7;
            n->body = $9;
            $$ = n;
        }
    ;

repeat_stmt
    : REPEAT LPAREN expr RPAREN block
        {
            Node *n = create_node(NODE_REPEAT);
            n->cond = $3;
            n->body = $5;
            $$ = n;
        }
    ;

expr
    : INTEGER_LITERAL
        { Node *n = create_node(NODE_NUM); n->val = $1; $$ = n; }
    | FLOAT_LITERAL
        { Node *n = create_node(NODE_NUM); n->val = $1; $$ = n; }
    | IDENTIFIER
        { Node *n = create_node(NODE_VAR); n->id = $1; $$ = n; }
    | IDENTIFIER LBRACK expr RBRACK
        {
            Node *n = create_node(NODE_ARRAY_REF);
            n->id    = $1;
            n->index = $3;
            $$ = n;
        }
    | IDENTIFIER LPAREN RPAREN
        {
            Node *n = create_node(NODE_CALL);
            n->id = $1;
            $$ = n;
        }
    | YES
        { Node *n = create_node(NODE_NUM); n->val = 1.0; $$ = n; }
    | NO
        { Node *n = create_node(NODE_NUM); n->val = 0.0; $$ = n; }
    | expr PLUS  expr  { Node *n=create_node(NODE_OP); n->op='+'; n->left=$1; n->right=$3; $$=n; }
    | expr MINUS expr  { Node *n=create_node(NODE_OP); n->op='-'; n->left=$1; n->right=$3; $$=n; }
    | expr MUL   expr  { Node *n=create_node(NODE_OP); n->op='*'; n->left=$1; n->right=$3; $$=n; }
    | expr DIV   expr  { Node *n=create_node(NODE_OP); n->op='/'; n->left=$1; n->right=$3; $$=n; }
    | expr GT    expr  { Node *n=create_node(NODE_OP); n->op='>'; n->left=$1; n->right=$3; $$=n; }
    | expr LT    expr  { Node *n=create_node(NODE_OP); n->op='<'; n->left=$1; n->right=$3; $$=n; }
    | expr GTE   expr  { Node *n=create_node(NODE_OP); n->op='G'; n->left=$1; n->right=$3; $$=n; }
    | expr LTE   expr  { Node *n=create_node(NODE_OP); n->op='L'; n->left=$1; n->right=$3; $$=n; }
    | expr EQ    expr  { Node *n=create_node(NODE_OP); n->op='E'; n->left=$1; n->right=$3; $$=n; }
    | expr NEQ   expr  { Node *n=create_node(NODE_OP); n->op='N'; n->left=$1; n->right=$3; $$=n; }
    | MINUS expr %prec UMINUS
        {
            Node *zero = create_node(NODE_NUM); zero->val = 0.0;
            Node *n    = create_node(NODE_OP);
            n->op    = '-';
            n->left  = zero;
            n->right = $2;
            $$ = n;
        }
    | LPAREN expr RPAREN
        { $$ = $2; }
    ;

%%

/* ── Error handler ──────────────────────────────────────── */
void yyerror(const char *s) {
    fprintf(stderr, "Syntax Error at line %d: %s\n", yylineno, s);
}

/* ── Entry point ────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    const char *in_file  = "input.mpp";
    const char *out_file = "output.txt";

    if (argc >= 2) in_file  = argv[1];
    if (argc >= 3) out_file = argv[2];

    yyin = fopen(in_file, "r");
    if (!yyin) {
        fprintf(stderr, "Error: Cannot open '%s'\n", in_file);
        return 1;
    }

    output_file = fopen(out_file, "w");
    if (!output_file) {
        fprintf(stderr, "Error: Cannot open '%s' for writing\n", out_file);
        fclose(yyin);
        return 1;
    }

    yyparse();

    fclose(yyin);
    fclose(output_file);

    /* Also mirror output to terminal */
    FILE *f = fopen(out_file, "r");
    if (f) {
        char line[512];
        printf("\n=== Program Output ===\n");
        while (fgets(line, sizeof(line), f)) printf("%s", line);
        fclose(f);
    }
    return 0;
}
