%{
/*=============================================================
  mpp.y  -  Parser + AST + Tree-Walking Interpreter for M++

  ADDITIONS over baseline:
    - NODE_MATHFUNC node type for built-in math functions
    - Built-in functions: msin, mcos, mtan, mlog, msqrt, mpow
      * All trig functions accept values in RADIANS
      * mlog  = log base-10
      * msqrt = square root
      * mpow(base, exp) = base raised to exp
    - Grammar rules for single-arg and two-arg math calls
    - NODE_MATHFUNC eval in eval()
    - NODE_MATHFUNC no-op in execute() (expression-only node)

  ORIGINAL FIXES retained:
    1. NODE_RETURN / NODE_BREAK / NODE_CONTINUE  control flow
    2. g_return_flag / g_break_flag / g_continue_flag globals
    3. FLOAT_LITERAL in expr rule
    4. Function calls inside expressions (NODE_CALL in eval)
    5. else-if chain "otherwise when(...)"
    6. Empty block "start finish" no longer crashes
    7. NODE_STMT_LIST short-circuits on return/break/continue
    8. show() prints integers without trailing .0
    9. Negative numbers via UMINUS precedence
   10. NEQ / GTE / LTE operators
   11. char literal stored as ASCII value
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

/* -- Control-flow flags ----------------------------------- */
int    g_return_flag   = 0;
double g_return_val    = 0.0;
int    g_break_flag    = 0;
int    g_continue_flag = 0;

/* -- AST node types --------------------------------------- */
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
    NODE_MATHFUNC,    /* built-in math: msin mcos mtan mlog msqrt mpow */
    NODE_VAR_DECL,
    NODE_ARRAY_DECL,
    NODE_ARRAY_ASSIGN,
    NODE_ARRAY_REF,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE
} NodeType;

typedef struct Node {
    NodeType    type;
    int         op;
    char       *id;
    char       *str;
    double      val;
    struct Node *left;
    struct Node *right;
    struct Node *cond;
    struct Node *step;
    struct Node *body;
    struct Node *else_part;
    struct Node *next;
    struct Node *index;
} Node;

/* -- Symbol table ----------------------------------------- */
typedef struct {
    char   *name;
    double  val;
    double *array;
    int     size;
    int     is_char;
} Symbol;

#define MAX_SYMBOLS 1000
Symbol var_table[MAX_SYMBOLS];
int    var_count = 0;

/* -- Function table --------------------------------------- */
typedef struct { char *name; Node *body; } Function;
#define MAX_FUNCS 100
Function func_table[MAX_FUNCS];
int      func_count = 0;

/* -- Symbol helpers --------------------------------------- */
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
    /* If a scalar with the same name already exists, upgrade it to array */
    Symbol *s = find_symbol(name);
    if (s) {
        /* Re-use the slot, allocate fresh array storage */
        free(s->array);
        s->array   = calloc(size, sizeof(double));
        s->size    = size;
        s->val     = 0;
        s->is_char = 0;
        return;
    }
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
    else
        fprintf(stderr, "Warning: array '%s' index %d out of range\n", name, idx);
}

double get_array_val(const char *name, int idx) {
    Symbol *s = find_symbol(name);
    if (s && s->array && idx >= 0 && idx < s->size)
        return s->array[idx];
    fprintf(stderr, "Warning: array '%s' index %d out of range\n", name, idx);
    return 0.0;
}

/* -- Function helpers ------------------------------------- */
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

/* -- AST node constructor --------------------------------- */
Node *create_node(NodeType type) {
    Node *n = calloc(1, sizeof(Node));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    n->type = type;
    return n;
}

/* -- Forward declarations --------------------------------- */
double execute(Node *n);
double eval(Node *n);

/* ===========================================================
   eval() - Expression evaluator
   =========================================================== */
double eval(Node *n) {
    if (!n) return 0.0;
    switch (n->type) {

        case NODE_NUM:
            return n->val;

        case NODE_VAR:
            return get_var(n->id);

        case NODE_ARRAY_REF:
            return get_array_val(n->id, (int)eval(n->index));

        /* -- Zero-argument user function call ----------- */
        case NODE_CALL: {
            int    saved_ret   = g_return_flag;
            double saved_rval  = g_return_val;
            int    saved_brk   = g_break_flag;
            int    saved_cont  = g_continue_flag;
            g_return_flag = g_break_flag = g_continue_flag = 0;
            g_return_val  = 0.0;

            Node *body = find_func_node(n->id);
            if (body) execute(body);
            double ret = g_return_val;

            g_return_flag   = saved_ret;
            g_return_val    = saved_rval;
            g_break_flag    = saved_brk;
            g_continue_flag = saved_cont;
            return ret;
        }

        /* -- Built-in math function call --------------- */
        case NODE_MATHFUNC: {
            double arg1 = eval(n->left);
            double arg2 = n->right ? eval(n->right) : 0.0;
            const char *fname = n->id;

            if (strcmp(fname, "msin")  == 0) return sin(arg1);
            if (strcmp(fname, "mcos")  == 0) return cos(arg1);
            if (strcmp(fname, "mtan")  == 0) return tan(arg1);
            if (strcmp(fname, "mlog")  == 0) {
                if (arg1 <= 0.0) {
                    fprintf(stderr, "Warning: mlog(%g) undefined\n", arg1);
                    return 0.0;
                }
                return log10(arg1);
            }
            if (strcmp(fname, "msqrt") == 0) {
                if (arg1 < 0.0) {
                    fprintf(stderr, "Warning: msqrt(%g) undefined\n", arg1);
                    return 0.0;
                }
                return sqrt(arg1);
            }
            if (strcmp(fname, "mpow")  == 0) return pow(arg1, arg2);
            fprintf(stderr, "Warning: unknown math function '%s'\n", fname);
            return 0.0;
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

/* ===========================================================
   execute() - Statement executor
   =========================================================== */
double execute(Node *n) {
    if (!n) return 0.0;

    if (g_return_flag || g_break_flag || g_continue_flag)
        return g_return_val;

    switch (n->type) {

        case NODE_STMT_LIST:
            execute(n->left);
            if (!g_return_flag && !g_break_flag && !g_continue_flag)
                execute(n->next);
            break;

        case NODE_VAR_DECL:
            if (n->str) {
                set_char_var(n->id, n->str[1]);
            } else if (n->left) {
                set_var(n->id, eval(n->left));
            } else {
                set_var(n->id, 0.0);
            }
            break;

        case NODE_ASSIGN:
            if (n->str) {
                set_char_var(n->id, n->str[1]);
            } else {
                set_var(n->id, eval(n->left));
            }
            break;

        case NODE_ARRAY_DECL:
            init_array(n->id, (int)n->val);
            break;

        case NODE_ARRAY_ASSIGN:
            set_array_val(n->id, (int)eval(n->index), eval(n->left));
            break;

        case NODE_TAKE: {
            double v;
            printf("Enter value for %s: ", n->id);
            fflush(stdout);
            if (scanf("%lf", &v) == 1)
                set_var(n->id, v);
            break;
        }

        case NODE_SHOW:
            if (n->str) {
                int len = strlen(n->str);
                char *s = malloc(len + 1);
                strncpy(s, n->str + 1, len - 2);
                s[len - 2] = '\0';
                fprintf(output_file, "%s\n", s);
                fflush(output_file);
                free(s);
            } else {
                double v = eval(n->left);
                if (n->left && n->left->type == NODE_VAR) {
                    Symbol *sym = find_symbol(n->left->id);
                    if (sym && sym->is_char) {
                        fprintf(output_file, "%c\n", (char)(int)v);
                        fflush(output_file);
                        break;
                    }
                }
                if (v == (double)(long long)v && fabs(v) < 1e15)
                    fprintf(output_file, "%lld\n", (long long)v);
                else
                    fprintf(output_file, "%g\n", v);
                fflush(output_file);
            }
            break;

        case NODE_IF:
            if (eval(n->cond))
                execute(n->body);
            else if (n->else_part)
                execute(n->else_part);
            break;

        case NODE_LOOP:
            execute(n->left);   /* init */
            while (!g_return_flag && eval(n->cond)) {
                execute(n->body);
                if (g_break_flag)    { g_break_flag = 0;    break; }
                if (g_continue_flag) { g_continue_flag = 0; }
                if (g_return_flag)   break;
                execute(n->step);
            }
            break;

        case NODE_REPEAT:
            while (!g_return_flag && eval(n->cond)) {
                execute(n->body);
                if (g_break_flag)    { g_break_flag = 0;    break; }
                if (g_continue_flag) { g_continue_flag = 0; continue; }
                if (g_return_flag)   break;
            }
            break;

        case NODE_RETURN:
            g_return_val  = n->left ? eval(n->left) : 0.0;
            g_return_flag = 1;
            break;

        case NODE_BREAK:
            g_break_flag = 1;
            break;

        case NODE_CONTINUE:
            g_continue_flag = 1;
            break;

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

        /* Math function used as statement (result discarded) */
        case NODE_MATHFUNC:
            eval(n);
            break;

        default: break;
    }
    return g_return_val;
}

%}

/* -- Bison union & token declarations --------------------- */
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

/* -- Math built-in function tokens ----------------------- */
%token MSIN MCOS MTAN MLOG MSQRT MPOW

%type <node> expr statement statements block
%type <node> declaration assignment show_stmt take_stmt
%type <node> if_stmt loop_stmt repeat_stmt

/* -- Operator precedence (low -> high) --------------------- */
%left  EQ NEQ
%left  GT LT GTE LTE
%left  PLUS MINUS
%left  MUL DIV
%right UMINUS

%%

/* ==========================================================
   Grammar Rules
   ========================================================== */

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

/* ==========================================================
   Expression rules
   ========================================================== */
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

    /* -- Single-argument math built-ins ------------------- */
    | MSIN  LPAREN expr RPAREN
        {
            Node *n = create_node(NODE_MATHFUNC);
            n->id   = strdup("msin");
            n->left = $3;
            $$ = n;
        }
    | MCOS  LPAREN expr RPAREN
        {
            Node *n = create_node(NODE_MATHFUNC);
            n->id   = strdup("mcos");
            n->left = $3;
            $$ = n;
        }
    | MTAN  LPAREN expr RPAREN
        {
            Node *n = create_node(NODE_MATHFUNC);
            n->id   = strdup("mtan");
            n->left = $3;
            $$ = n;
        }
    | MLOG  LPAREN expr RPAREN
        {
            Node *n = create_node(NODE_MATHFUNC);
            n->id   = strdup("mlog");
            n->left = $3;
            $$ = n;
        }
    | MSQRT LPAREN expr RPAREN
        {
            Node *n = create_node(NODE_MATHFUNC);
            n->id   = strdup("msqrt");
            n->left = $3;
            $$ = n;
        }

    /* -- Two-argument built-in: mpow(base, exp) ----------- */
    | MPOW  LPAREN expr COMMA expr RPAREN
        {
            Node *n  = create_node(NODE_MATHFUNC);
            n->id    = strdup("mpow");
            n->left  = $3;    /* base */
            n->right = $5;    /* exponent */
            $$ = n;
        }

    /* -- Arithmetic & comparison operators ---------------- */
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

/* -- Error handler ---------------------------------------- */
void yyerror(const char *s) {
    fprintf(stderr, "Syntax Error at line %d: %s\n", yylineno, s);
}

/* -- Entry point ------------------------------------------ */
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

    FILE *f = fopen(out_file, "r");
    if (f) {
        char line[512];
        printf("\n=== Program Output ===\n");
        while (fgets(line, sizeof(line), f)) printf("%s", line);
        fclose(f);
    }
    return 0;
}
