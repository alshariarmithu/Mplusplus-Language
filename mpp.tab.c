/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "mpp.y"

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


#line 508 "mpp.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "mpp.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INTEGER_LITERAL = 3,            /* INTEGER_LITERAL  */
  YYSYMBOL_FLOAT_LITERAL = 4,              /* FLOAT_LITERAL  */
  YYSYMBOL_IDENTIFIER = 5,                 /* IDENTIFIER  */
  YYSYMBOL_STRING_LITERAL = 6,             /* STRING_LITERAL  */
  YYSYMBOL_CHAR_LITERAL = 7,               /* CHAR_LITERAL  */
  YYSYMBOL_START = 8,                      /* START  */
  YYSYMBOL_FINISH = 9,                     /* FINISH  */
  YYSYMBOL_WHEN = 10,                      /* WHEN  */
  YYSYMBOL_OTHERWISE = 11,                 /* OTHERWISE  */
  YYSYMBOL_REPEAT = 12,                    /* REPEAT  */
  YYSYMBOL_LOOP = 13,                      /* LOOP  */
  YYSYMBOL_SHOW = 14,                      /* SHOW  */
  YYSYMBOL_TAKE = 15,                      /* TAKE  */
  YYSYMBOL_TYPE_NUMBER = 16,               /* TYPE_NUMBER  */
  YYSYMBOL_TYPE_DECIMAL = 17,              /* TYPE_DECIMAL  */
  YYSYMBOL_TYPE_LETTER = 18,               /* TYPE_LETTER  */
  YYSYMBOL_DEFINE = 19,                    /* DEFINE  */
  YYSYMBOL_DORETURN = 20,                  /* DORETURN  */
  YYSYMBOL_NOTHING = 21,                   /* NOTHING  */
  YYSYMBOL_MAIN = 22,                      /* MAIN  */
  YYSYMBOL_YES = 23,                       /* YES  */
  YYSYMBOL_NO = 24,                        /* NO  */
  YYSYMBOL_STOP = 25,                      /* STOP  */
  YYSYMBOL_SKIP = 26,                      /* SKIP  */
  YYSYMBOL_ASSIGN = 27,                    /* ASSIGN  */
  YYSYMBOL_SEMICOLON = 28,                 /* SEMICOLON  */
  YYSYMBOL_LPAREN = 29,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 30,                    /* RPAREN  */
  YYSYMBOL_LBRACK = 31,                    /* LBRACK  */
  YYSYMBOL_RBRACK = 32,                    /* RBRACK  */
  YYSYMBOL_PLUS = 33,                      /* PLUS  */
  YYSYMBOL_MINUS = 34,                     /* MINUS  */
  YYSYMBOL_MUL = 35,                       /* MUL  */
  YYSYMBOL_DIV = 36,                       /* DIV  */
  YYSYMBOL_GT = 37,                        /* GT  */
  YYSYMBOL_LT = 38,                        /* LT  */
  YYSYMBOL_GTE = 39,                       /* GTE  */
  YYSYMBOL_LTE = 40,                       /* LTE  */
  YYSYMBOL_EQ = 41,                        /* EQ  */
  YYSYMBOL_NEQ = 42,                       /* NEQ  */
  YYSYMBOL_COMMA = 43,                     /* COMMA  */
  YYSYMBOL_MSIN = 44,                      /* MSIN  */
  YYSYMBOL_MCOS = 45,                      /* MCOS  */
  YYSYMBOL_MTAN = 46,                      /* MTAN  */
  YYSYMBOL_MLOG = 47,                      /* MLOG  */
  YYSYMBOL_MSQRT = 48,                     /* MSQRT  */
  YYSYMBOL_MPOW = 49,                      /* MPOW  */
  YYSYMBOL_UMINUS = 50,                    /* UMINUS  */
  YYSYMBOL_YYACCEPT = 51,                  /* $accept  */
  YYSYMBOL_program = 52,                   /* program  */
  YYSYMBOL_elements = 53,                  /* elements  */
  YYSYMBOL_element = 54,                   /* element  */
  YYSYMBOL_type_spec = 55,                 /* type_spec  */
  YYSYMBOL_block = 56,                     /* block  */
  YYSYMBOL_statements = 57,                /* statements  */
  YYSYMBOL_statement = 58,                 /* statement  */
  YYSYMBOL_declaration = 59,               /* declaration  */
  YYSYMBOL_assignment = 60,                /* assignment  */
  YYSYMBOL_show_stmt = 61,                 /* show_stmt  */
  YYSYMBOL_take_stmt = 62,                 /* take_stmt  */
  YYSYMBOL_if_stmt = 63,                   /* if_stmt  */
  YYSYMBOL_loop_stmt = 64,                 /* loop_stmt  */
  YYSYMBOL_repeat_stmt = 65,               /* repeat_stmt  */
  YYSYMBOL_expr = 66                       /* expr  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   427

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  51
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  16
/* YYNRULES -- Number of rules.  */
#define YYNRULES  67
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  161

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   305


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   475,   475,   488,   489,   493,   495,   497,   502,   503,
     504,   508,   509,   513,   515,   531,   532,   533,   534,   535,
     536,   537,   538,   539,   545,   551,   553,   555,   564,   571,
     578,   585,   595,   602,   609,   620,   626,   635,   644,   651,
     659,   670,   682,   695,   697,   699,   701,   708,   714,   716,
     720,   727,   734,   741,   748,   757,   767,   768,   769,   770,
     771,   772,   773,   774,   775,   776,   777,   786
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INTEGER_LITERAL",
  "FLOAT_LITERAL", "IDENTIFIER", "STRING_LITERAL", "CHAR_LITERAL", "START",
  "FINISH", "WHEN", "OTHERWISE", "REPEAT", "LOOP", "SHOW", "TAKE",
  "TYPE_NUMBER", "TYPE_DECIMAL", "TYPE_LETTER", "DEFINE", "DORETURN",
  "NOTHING", "MAIN", "YES", "NO", "STOP", "SKIP", "ASSIGN", "SEMICOLON",
  "LPAREN", "RPAREN", "LBRACK", "RBRACK", "PLUS", "MINUS", "MUL", "DIV",
  "GT", "LT", "GTE", "LTE", "EQ", "NEQ", "COMMA", "MSIN", "MCOS", "MTAN",
  "MLOG", "MSQRT", "MPOW", "UMINUS", "$accept", "program", "elements",
  "element", "type_spec", "block", "statements", "statement",
  "declaration", "assignment", "show_stmt", "take_stmt", "if_stmt",
  "loop_stmt", "repeat_stmt", "expr", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-43)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -43,    14,     3,   -43,    -6,   -43,   -43,   -43,   -43,     8,
      27,    15,    17,    18,    24,    40,    45,    75,    75,    75,
      11,   -43,   -43,   -43,    12,   -43,    56,    59,    89,   102,
      88,    63,   105,   109,   127,   -43,    64,   -43,   110,   111,
     117,   119,   -43,   -43,   -43,   112,   118,   175,   175,   175,
     104,   120,   -43,   -43,   -43,     4,   -43,   -43,   -43,   175,
     175,   121,   133,   134,   141,   144,   145,   197,   -43,   -43,
     -23,   -43,   -43,   -43,   -43,   -43,   -43,   -43,   385,   147,
     353,   225,   238,   153,   146,   251,   154,   175,   264,   -43,
     175,   175,   175,   175,   175,   175,   -43,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   148,   137,   -43,
     156,    75,    75,   175,   -43,   -43,   -43,   364,   -43,   277,
     290,   303,   316,   329,   374,    78,    78,   -43,   -43,    94,
      94,    94,    94,    65,    65,   -43,   385,   155,   175,   174,
     -43,   212,   -43,   -43,   -43,   -43,   -43,   -43,   175,   -43,
     385,    32,   181,   342,   -43,   -43,     7,   158,   -43,    75,
     -43
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       3,     0,     2,     1,     0,     4,     8,     9,    10,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     6,     7,     5,     0,    12,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    22,     0,    13,     0,     0,
       0,     0,    19,    20,    21,     0,     0,     0,     0,     0,
       0,     0,    37,    43,    44,    45,    48,    49,    24,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    25,    26,
      30,    11,    14,    15,    16,    17,    18,    33,    32,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    66,
       0,     0,     0,     0,     0,     0,    23,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    27,
       0,     0,     0,     0,    36,    35,    47,     0,    67,     0,
       0,     0,     0,     0,     0,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    29,    28,     0,     0,    38,
      42,     0,    46,    50,    51,    52,    53,    54,     0,    31,
      34,     0,     0,     0,    39,    40,     0,     0,    55,     0,
      41
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -43,   -43,   -43,   -43,   185,   -17,   -43,   164,   140,    39,
     -43,   -43,    50,   -43,   -43,   -42
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     1,     2,     5,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    67
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      21,    22,    23,    78,   107,    80,    81,    82,   108,    85,
       6,     7,     8,    11,     3,     9,    24,    88,    89,    20,
      25,    26,     4,    27,    28,    29,    30,     6,     7,     8,
      12,    31,    13,    86,    45,    87,    32,    33,    47,    45,
      20,    46,    26,    47,    14,   117,    15,    16,   119,   120,
     121,   122,   123,   124,    17,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   136,    53,    54,    55,    24,
      18,   141,    20,    71,    26,    19,    27,    28,    29,    30,
       6,     7,     8,    20,    31,    48,    56,    57,    49,    32,
      33,    58,    59,    52,   139,   140,   150,    60,    97,    98,
      99,   100,   101,   102,   103,   104,   153,    61,    62,    63,
      64,    65,    66,    99,   100,    53,    54,    55,    50,    77,
       6,     7,     8,    53,    54,    55,    84,    97,    98,    99,
     100,    51,    70,    68,   154,    56,    57,    69,    73,    74,
     137,    59,   160,    56,    57,    75,    60,    76,    79,    59,
      90,    53,    54,    55,    60,   135,    61,    62,    63,    64,
      65,    66,    91,    92,    61,    62,    63,    64,    65,    66,
      93,    56,    57,    94,    95,   109,   114,    59,    53,    54,
      55,   113,    60,   138,   116,   151,   156,   149,   159,    10,
      83,   157,    61,    62,    63,    64,    65,    66,    56,    57,
      72,   155,     0,     0,    59,     0,     0,     0,     0,    60,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    61,
      62,    63,    64,    65,    66,    96,     0,     0,     0,     0,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     152,     0,     0,     0,     0,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   111,     0,     0,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   112,     0,
       0,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   115,     0,     0,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   118,     0,     0,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   143,     0,     0,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     144,     0,     0,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   145,     0,     0,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   146,     0,     0,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   147,
       0,     0,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   158,     0,     0,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   110,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   142,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   148,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106
};

static const yytype_int16 yycheck[] =
{
      17,    18,    19,    45,    27,    47,    48,    49,    31,    51,
      16,    17,    18,     5,     0,    21,     5,    59,    60,     8,
       9,    10,    19,    12,    13,    14,    15,    16,    17,    18,
      22,    20,     5,    29,    27,    31,    25,    26,    31,    27,
       8,    29,    10,    31,    29,    87,    29,    29,    90,    91,
      92,    93,    94,    95,    30,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,     3,     4,     5,     5,
      30,   113,     8,     9,    10,    30,    12,    13,    14,    15,
      16,    17,    18,     8,    20,    29,    23,    24,    29,    25,
      26,    28,    29,     5,   111,   112,   138,    34,    33,    34,
      35,    36,    37,    38,    39,    40,   148,    44,    45,    46,
      47,    48,    49,    35,    36,     3,     4,     5,    29,     7,
      16,    17,    18,     3,     4,     5,     6,    33,    34,    35,
      36,    29,     5,    28,   151,    23,    24,    28,    28,    28,
       3,    29,   159,    23,    24,    28,    34,    28,    30,    29,
      29,     3,     4,     5,    34,     7,    44,    45,    46,    47,
      48,    49,    29,    29,    44,    45,    46,    47,    48,    49,
      29,    23,    24,    29,    29,    28,    30,    29,     3,     4,
       5,    28,    34,    27,    30,    11,     5,    32,    30,     4,
      50,   152,    44,    45,    46,    47,    48,    49,    23,    24,
      36,   151,    -1,    -1,    29,    -1,    -1,    -1,    -1,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,
      45,    46,    47,    48,    49,    28,    -1,    -1,    -1,    -1,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      28,    -1,    -1,    -1,    -1,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    30,    -1,    -1,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    30,    -1,
      -1,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    30,    -1,    -1,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    30,    -1,    -1,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    30,    -1,    -1,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      30,    -1,    -1,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    30,    -1,    -1,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    30,    -1,    -1,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    30,
      -1,    -1,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    30,    -1,    -1,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    52,    53,     0,    19,    54,    16,    17,    18,    21,
      55,     5,    22,     5,    29,    29,    29,    30,    30,    30,
       8,    56,    56,    56,     5,     9,    10,    12,    13,    14,
      15,    20,    25,    26,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    27,    29,    31,    29,    29,
      29,    29,     5,     3,     4,     5,    23,    24,    28,    29,
      34,    44,    45,    46,    47,    48,    49,    66,    28,    28,
       5,     9,    58,    28,    28,    28,    28,     7,    66,    30,
      66,    66,    66,    59,     6,    66,    29,    31,    66,    66,
      29,    29,    29,    29,    29,    29,    28,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    27,    31,    28,
      32,    30,    30,    28,    30,    30,    30,    66,    30,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,     7,    66,     3,    27,    56,
      56,    66,    32,    30,    30,    30,    30,    30,    43,    32,
      66,    11,    28,    66,    56,    63,     5,    60,    30,    30,
      56
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    51,    52,    53,    53,    54,    54,    54,    55,    55,
      55,    56,    56,    57,    57,    58,    58,    58,    58,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    59,    59,
      59,    59,    60,    60,    60,    61,    61,    62,    63,    63,
      63,    64,    65,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    66,    66,    66
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     0,     2,     6,     6,     6,     1,     1,
       1,     3,     2,     1,     2,     2,     2,     2,     2,     1,
       1,     1,     1,     3,     2,     2,     2,     4,     4,     4,
       2,     5,     3,     3,     6,     4,     4,     2,     5,     7,
       7,     9,     5,     1,     1,     1,     4,     3,     1,     1,
       4,     4,     4,     4,     4,     6,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: elements  */
#line 476 "mpp.y"
        {
            Node *m = find_func_node("main");
            if (m) {
                g_return_flag = g_break_flag = g_continue_flag = 0;
                execute(m);
            } else {
                fprintf(stderr, "Error: no 'main' function found.\n");
            }
        }
#line 1721 "mpp.tab.c"
    break;

  case 5: /* element: DEFINE type_spec IDENTIFIER LPAREN RPAREN block  */
#line 494 "mpp.y"
        { register_func((yyvsp[-3].strval), (yyvsp[0].node)); }
#line 1727 "mpp.tab.c"
    break;

  case 6: /* element: DEFINE NOTHING IDENTIFIER LPAREN RPAREN block  */
#line 496 "mpp.y"
        { register_func((yyvsp[-3].strval), (yyvsp[0].node)); }
#line 1733 "mpp.tab.c"
    break;

  case 7: /* element: DEFINE NOTHING MAIN LPAREN RPAREN block  */
#line 498 "mpp.y"
        { register_func("main", (yyvsp[0].node)); }
#line 1739 "mpp.tab.c"
    break;

  case 11: /* block: START statements FINISH  */
#line 508 "mpp.y"
                               { (yyval.node) = (yyvsp[-1].node); }
#line 1745 "mpp.tab.c"
    break;

  case 12: /* block: START FINISH  */
#line 509 "mpp.y"
                               { (yyval.node) = NULL; }
#line 1751 "mpp.tab.c"
    break;

  case 13: /* statements: statement  */
#line 514 "mpp.y"
        { (yyval.node) = (yyvsp[0].node); }
#line 1757 "mpp.tab.c"
    break;

  case 14: /* statements: statements statement  */
#line 516 "mpp.y"
        {
            if ((yyvsp[-1].node) == NULL) {
                (yyval.node) = (yyvsp[0].node);
            } else if ((yyvsp[0].node) == NULL) {
                (yyval.node) = (yyvsp[-1].node);
            } else {
                Node *n = create_node(NODE_STMT_LIST);
                n->left = (yyvsp[-1].node);
                n->next = (yyvsp[0].node);
                (yyval.node) = n;
            }
        }
#line 1774 "mpp.tab.c"
    break;

  case 15: /* statement: declaration SEMICOLON  */
#line 531 "mpp.y"
                                    { (yyval.node) = (yyvsp[-1].node); }
#line 1780 "mpp.tab.c"
    break;

  case 16: /* statement: assignment SEMICOLON  */
#line 532 "mpp.y"
                                    { (yyval.node) = (yyvsp[-1].node); }
#line 1786 "mpp.tab.c"
    break;

  case 17: /* statement: show_stmt SEMICOLON  */
#line 533 "mpp.y"
                                    { (yyval.node) = (yyvsp[-1].node); }
#line 1792 "mpp.tab.c"
    break;

  case 18: /* statement: take_stmt SEMICOLON  */
#line 534 "mpp.y"
                                    { (yyval.node) = (yyvsp[-1].node); }
#line 1798 "mpp.tab.c"
    break;

  case 19: /* statement: if_stmt  */
#line 535 "mpp.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 1804 "mpp.tab.c"
    break;

  case 20: /* statement: loop_stmt  */
#line 536 "mpp.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 1810 "mpp.tab.c"
    break;

  case 21: /* statement: repeat_stmt  */
#line 537 "mpp.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 1816 "mpp.tab.c"
    break;

  case 22: /* statement: block  */
#line 538 "mpp.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 1822 "mpp.tab.c"
    break;

  case 23: /* statement: DORETURN expr SEMICOLON  */
#line 540 "mpp.y"
        {
            Node *n = create_node(NODE_RETURN);
            n->left = (yyvsp[-1].node);
            (yyval.node) = n;
        }
#line 1832 "mpp.tab.c"
    break;

  case 24: /* statement: DORETURN SEMICOLON  */
#line 546 "mpp.y"
        {
            Node *n = create_node(NODE_RETURN);
            n->left = NULL;
            (yyval.node) = n;
        }
#line 1842 "mpp.tab.c"
    break;

  case 25: /* statement: STOP SEMICOLON  */
#line 552 "mpp.y"
        { (yyval.node) = create_node(NODE_BREAK); }
#line 1848 "mpp.tab.c"
    break;

  case 26: /* statement: SKIP SEMICOLON  */
#line 554 "mpp.y"
        { (yyval.node) = create_node(NODE_CONTINUE); }
#line 1854 "mpp.tab.c"
    break;

  case 27: /* statement: IDENTIFIER LPAREN RPAREN SEMICOLON  */
#line 556 "mpp.y"
        {
            Node *n = create_node(NODE_CALL);
            n->id = (yyvsp[-3].strval);
            (yyval.node) = n;
        }
#line 1864 "mpp.tab.c"
    break;

  case 28: /* declaration: type_spec IDENTIFIER ASSIGN expr  */
#line 565 "mpp.y"
        {
            Node *n = create_node(NODE_VAR_DECL);
            n->id   = (yyvsp[-2].strval);
            n->left = (yyvsp[0].node);
            (yyval.node) = n;
        }
#line 1875 "mpp.tab.c"
    break;

  case 29: /* declaration: type_spec IDENTIFIER ASSIGN CHAR_LITERAL  */
#line 572 "mpp.y"
        {
            Node *n = create_node(NODE_VAR_DECL);
            n->id  = (yyvsp[-2].strval);
            n->str = (yyvsp[0].strval);
            (yyval.node) = n;
        }
#line 1886 "mpp.tab.c"
    break;

  case 30: /* declaration: type_spec IDENTIFIER  */
#line 579 "mpp.y"
        {
            Node *n = create_node(NODE_VAR_DECL);
            n->id   = (yyvsp[0].strval);
            n->left = NULL;
            (yyval.node) = n;
        }
#line 1897 "mpp.tab.c"
    break;

  case 31: /* declaration: type_spec IDENTIFIER LBRACK INTEGER_LITERAL RBRACK  */
#line 586 "mpp.y"
        {
            Node *n = create_node(NODE_ARRAY_DECL);
            n->id  = (yyvsp[-3].strval);
            n->val = (yyvsp[-1].dval);
            (yyval.node) = n;
        }
#line 1908 "mpp.tab.c"
    break;

  case 32: /* assignment: IDENTIFIER ASSIGN expr  */
#line 596 "mpp.y"
        {
            Node *n = create_node(NODE_ASSIGN);
            n->id   = (yyvsp[-2].strval);
            n->left = (yyvsp[0].node);
            (yyval.node) = n;
        }
#line 1919 "mpp.tab.c"
    break;

  case 33: /* assignment: IDENTIFIER ASSIGN CHAR_LITERAL  */
#line 603 "mpp.y"
        {
            Node *n = create_node(NODE_ASSIGN);
            n->id  = (yyvsp[-2].strval);
            n->str = (yyvsp[0].strval);
            (yyval.node) = n;
        }
#line 1930 "mpp.tab.c"
    break;

  case 34: /* assignment: IDENTIFIER LBRACK expr RBRACK ASSIGN expr  */
#line 610 "mpp.y"
        {
            Node *n = create_node(NODE_ARRAY_ASSIGN);
            n->id    = (yyvsp[-5].strval);
            n->index = (yyvsp[-3].node);
            n->left  = (yyvsp[0].node);
            (yyval.node) = n;
        }
#line 1942 "mpp.tab.c"
    break;

  case 35: /* show_stmt: SHOW LPAREN expr RPAREN  */
#line 621 "mpp.y"
        {
            Node *n = create_node(NODE_SHOW);
            n->left = (yyvsp[-1].node);
            (yyval.node) = n;
        }
#line 1952 "mpp.tab.c"
    break;

  case 36: /* show_stmt: SHOW LPAREN STRING_LITERAL RPAREN  */
#line 627 "mpp.y"
        {
            Node *n = create_node(NODE_SHOW);
            n->str = (yyvsp[-1].strval);
            (yyval.node) = n;
        }
#line 1962 "mpp.tab.c"
    break;

  case 37: /* take_stmt: TAKE IDENTIFIER  */
#line 636 "mpp.y"
        {
            Node *n = create_node(NODE_TAKE);
            n->id = (yyvsp[0].strval);
            (yyval.node) = n;
        }
#line 1972 "mpp.tab.c"
    break;

  case 38: /* if_stmt: WHEN LPAREN expr RPAREN block  */
#line 645 "mpp.y"
        {
            Node *n = create_node(NODE_IF);
            n->cond = (yyvsp[-2].node);
            n->body = (yyvsp[0].node);
            (yyval.node) = n;
        }
#line 1983 "mpp.tab.c"
    break;

  case 39: /* if_stmt: WHEN LPAREN expr RPAREN block OTHERWISE block  */
#line 652 "mpp.y"
        {
            Node *n = create_node(NODE_IF);
            n->cond      = (yyvsp[-4].node);
            n->body      = (yyvsp[-2].node);
            n->else_part = (yyvsp[0].node);
            (yyval.node) = n;
        }
#line 1995 "mpp.tab.c"
    break;

  case 40: /* if_stmt: WHEN LPAREN expr RPAREN block OTHERWISE if_stmt  */
#line 660 "mpp.y"
        {
            Node *n = create_node(NODE_IF);
            n->cond      = (yyvsp[-4].node);
            n->body      = (yyvsp[-2].node);
            n->else_part = (yyvsp[0].node);
            (yyval.node) = n;
        }
#line 2007 "mpp.tab.c"
    break;

  case 41: /* loop_stmt: LOOP LPAREN declaration SEMICOLON expr SEMICOLON assignment RPAREN block  */
#line 671 "mpp.y"
        {
            Node *n = create_node(NODE_LOOP);
            n->left = (yyvsp[-6].node);
            n->cond = (yyvsp[-4].node);
            n->step = (yyvsp[-2].node);
            n->body = (yyvsp[0].node);
            (yyval.node) = n;
        }
#line 2020 "mpp.tab.c"
    break;

  case 42: /* repeat_stmt: REPEAT LPAREN expr RPAREN block  */
#line 683 "mpp.y"
        {
            Node *n = create_node(NODE_REPEAT);
            n->cond = (yyvsp[-2].node);
            n->body = (yyvsp[0].node);
            (yyval.node) = n;
        }
#line 2031 "mpp.tab.c"
    break;

  case 43: /* expr: INTEGER_LITERAL  */
#line 696 "mpp.y"
        { Node *n = create_node(NODE_NUM); n->val = (yyvsp[0].dval); (yyval.node) = n; }
#line 2037 "mpp.tab.c"
    break;

  case 44: /* expr: FLOAT_LITERAL  */
#line 698 "mpp.y"
        { Node *n = create_node(NODE_NUM); n->val = (yyvsp[0].dval); (yyval.node) = n; }
#line 2043 "mpp.tab.c"
    break;

  case 45: /* expr: IDENTIFIER  */
#line 700 "mpp.y"
        { Node *n = create_node(NODE_VAR); n->id = (yyvsp[0].strval); (yyval.node) = n; }
#line 2049 "mpp.tab.c"
    break;

  case 46: /* expr: IDENTIFIER LBRACK expr RBRACK  */
#line 702 "mpp.y"
        {
            Node *n = create_node(NODE_ARRAY_REF);
            n->id    = (yyvsp[-3].strval);
            n->index = (yyvsp[-1].node);
            (yyval.node) = n;
        }
#line 2060 "mpp.tab.c"
    break;

  case 47: /* expr: IDENTIFIER LPAREN RPAREN  */
#line 709 "mpp.y"
        {
            Node *n = create_node(NODE_CALL);
            n->id = (yyvsp[-2].strval);
            (yyval.node) = n;
        }
#line 2070 "mpp.tab.c"
    break;

  case 48: /* expr: YES  */
#line 715 "mpp.y"
        { Node *n = create_node(NODE_NUM); n->val = 1.0; (yyval.node) = n; }
#line 2076 "mpp.tab.c"
    break;

  case 49: /* expr: NO  */
#line 717 "mpp.y"
        { Node *n = create_node(NODE_NUM); n->val = 0.0; (yyval.node) = n; }
#line 2082 "mpp.tab.c"
    break;

  case 50: /* expr: MSIN LPAREN expr RPAREN  */
#line 721 "mpp.y"
        {
            Node *n = create_node(NODE_MATHFUNC);
            n->id   = strdup("msin");
            n->left = (yyvsp[-1].node);
            (yyval.node) = n;
        }
#line 2093 "mpp.tab.c"
    break;

  case 51: /* expr: MCOS LPAREN expr RPAREN  */
#line 728 "mpp.y"
        {
            Node *n = create_node(NODE_MATHFUNC);
            n->id   = strdup("mcos");
            n->left = (yyvsp[-1].node);
            (yyval.node) = n;
        }
#line 2104 "mpp.tab.c"
    break;

  case 52: /* expr: MTAN LPAREN expr RPAREN  */
#line 735 "mpp.y"
        {
            Node *n = create_node(NODE_MATHFUNC);
            n->id   = strdup("mtan");
            n->left = (yyvsp[-1].node);
            (yyval.node) = n;
        }
#line 2115 "mpp.tab.c"
    break;

  case 53: /* expr: MLOG LPAREN expr RPAREN  */
#line 742 "mpp.y"
        {
            Node *n = create_node(NODE_MATHFUNC);
            n->id   = strdup("mlog");
            n->left = (yyvsp[-1].node);
            (yyval.node) = n;
        }
#line 2126 "mpp.tab.c"
    break;

  case 54: /* expr: MSQRT LPAREN expr RPAREN  */
#line 749 "mpp.y"
        {
            Node *n = create_node(NODE_MATHFUNC);
            n->id   = strdup("msqrt");
            n->left = (yyvsp[-1].node);
            (yyval.node) = n;
        }
#line 2137 "mpp.tab.c"
    break;

  case 55: /* expr: MPOW LPAREN expr COMMA expr RPAREN  */
#line 758 "mpp.y"
        {
            Node *n  = create_node(NODE_MATHFUNC);
            n->id    = strdup("mpow");
            n->left  = (yyvsp[-3].node);    /* base */
            n->right = (yyvsp[-1].node);    /* exponent */
            (yyval.node) = n;
        }
#line 2149 "mpp.tab.c"
    break;

  case 56: /* expr: expr PLUS expr  */
#line 767 "mpp.y"
                       { Node *n=create_node(NODE_OP); n->op='+'; n->left=(yyvsp[-2].node); n->right=(yyvsp[0].node); (yyval.node)=n; }
#line 2155 "mpp.tab.c"
    break;

  case 57: /* expr: expr MINUS expr  */
#line 768 "mpp.y"
                       { Node *n=create_node(NODE_OP); n->op='-'; n->left=(yyvsp[-2].node); n->right=(yyvsp[0].node); (yyval.node)=n; }
#line 2161 "mpp.tab.c"
    break;

  case 58: /* expr: expr MUL expr  */
#line 769 "mpp.y"
                       { Node *n=create_node(NODE_OP); n->op='*'; n->left=(yyvsp[-2].node); n->right=(yyvsp[0].node); (yyval.node)=n; }
#line 2167 "mpp.tab.c"
    break;

  case 59: /* expr: expr DIV expr  */
#line 770 "mpp.y"
                       { Node *n=create_node(NODE_OP); n->op='/'; n->left=(yyvsp[-2].node); n->right=(yyvsp[0].node); (yyval.node)=n; }
#line 2173 "mpp.tab.c"
    break;

  case 60: /* expr: expr GT expr  */
#line 771 "mpp.y"
                       { Node *n=create_node(NODE_OP); n->op='>'; n->left=(yyvsp[-2].node); n->right=(yyvsp[0].node); (yyval.node)=n; }
#line 2179 "mpp.tab.c"
    break;

  case 61: /* expr: expr LT expr  */
#line 772 "mpp.y"
                       { Node *n=create_node(NODE_OP); n->op='<'; n->left=(yyvsp[-2].node); n->right=(yyvsp[0].node); (yyval.node)=n; }
#line 2185 "mpp.tab.c"
    break;

  case 62: /* expr: expr GTE expr  */
#line 773 "mpp.y"
                       { Node *n=create_node(NODE_OP); n->op='G'; n->left=(yyvsp[-2].node); n->right=(yyvsp[0].node); (yyval.node)=n; }
#line 2191 "mpp.tab.c"
    break;

  case 63: /* expr: expr LTE expr  */
#line 774 "mpp.y"
                       { Node *n=create_node(NODE_OP); n->op='L'; n->left=(yyvsp[-2].node); n->right=(yyvsp[0].node); (yyval.node)=n; }
#line 2197 "mpp.tab.c"
    break;

  case 64: /* expr: expr EQ expr  */
#line 775 "mpp.y"
                       { Node *n=create_node(NODE_OP); n->op='E'; n->left=(yyvsp[-2].node); n->right=(yyvsp[0].node); (yyval.node)=n; }
#line 2203 "mpp.tab.c"
    break;

  case 65: /* expr: expr NEQ expr  */
#line 776 "mpp.y"
                       { Node *n=create_node(NODE_OP); n->op='N'; n->left=(yyvsp[-2].node); n->right=(yyvsp[0].node); (yyval.node)=n; }
#line 2209 "mpp.tab.c"
    break;

  case 66: /* expr: MINUS expr  */
#line 778 "mpp.y"
        {
            Node *zero = create_node(NODE_NUM); zero->val = 0.0;
            Node *n    = create_node(NODE_OP);
            n->op    = '-';
            n->left  = zero;
            n->right = (yyvsp[0].node);
            (yyval.node) = n;
        }
#line 2222 "mpp.tab.c"
    break;

  case 67: /* expr: LPAREN expr RPAREN  */
#line 787 "mpp.y"
        { (yyval.node) = (yyvsp[-1].node); }
#line 2228 "mpp.tab.c"
    break;


#line 2232 "mpp.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 790 "mpp.y"


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
