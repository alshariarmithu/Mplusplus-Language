/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_MPP_TAB_H_INCLUDED
# define YY_YY_MPP_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    INTEGER_LITERAL = 258,         /* INTEGER_LITERAL  */
    FLOAT_LITERAL = 259,           /* FLOAT_LITERAL  */
    IDENTIFIER = 260,              /* IDENTIFIER  */
    STRING_LITERAL = 261,          /* STRING_LITERAL  */
    CHAR_LITERAL = 262,            /* CHAR_LITERAL  */
    START = 263,                   /* START  */
    FINISH = 264,                  /* FINISH  */
    WHEN = 265,                    /* WHEN  */
    OTHERWISE = 266,               /* OTHERWISE  */
    REPEAT = 267,                  /* REPEAT  */
    LOOP = 268,                    /* LOOP  */
    SHOW = 269,                    /* SHOW  */
    TAKE = 270,                    /* TAKE  */
    TYPE_NUMBER = 271,             /* TYPE_NUMBER  */
    TYPE_DECIMAL = 272,            /* TYPE_DECIMAL  */
    TYPE_LETTER = 273,             /* TYPE_LETTER  */
    DEFINE = 274,                  /* DEFINE  */
    DORETURN = 275,                /* DORETURN  */
    NOTHING = 276,                 /* NOTHING  */
    MAIN = 277,                    /* MAIN  */
    YES = 278,                     /* YES  */
    NO = 279,                      /* NO  */
    STOP = 280,                    /* STOP  */
    SKIP = 281,                    /* SKIP  */
    ASSIGN = 282,                  /* ASSIGN  */
    SEMICOLON = 283,               /* SEMICOLON  */
    LPAREN = 284,                  /* LPAREN  */
    RPAREN = 285,                  /* RPAREN  */
    LBRACK = 286,                  /* LBRACK  */
    RBRACK = 287,                  /* RBRACK  */
    PLUS = 288,                    /* PLUS  */
    MINUS = 289,                   /* MINUS  */
    MUL = 290,                     /* MUL  */
    DIV = 291,                     /* DIV  */
    GT = 292,                      /* GT  */
    LT = 293,                      /* LT  */
    GTE = 294,                     /* GTE  */
    LTE = 295,                     /* LTE  */
    EQ = 296,                      /* EQ  */
    NEQ = 297,                     /* NEQ  */
    COMMA = 298,                   /* COMMA  */
    MSIN = 299,                    /* MSIN  */
    MCOS = 300,                    /* MCOS  */
    MTAN = 301,                    /* MTAN  */
    MLOG = 302,                    /* MLOG  */
    MSQRT = 303,                   /* MSQRT  */
    MPOW = 304,                    /* MPOW  */
    UMINUS = 305                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 439 "mpp.y"

    double      dval;
    char       *strval;
    struct Node *node;

#line 120 "mpp.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_MPP_TAB_H_INCLUDED  */
