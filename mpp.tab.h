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
    CHAR_LITERAL = 261,            /* CHAR_LITERAL  */
    STRING_LITERAL = 262,          /* STRING_LITERAL  */
    START = 263,                   /* START  */
    FINISH = 264,                  /* FINISH  */
    WHEN = 265,                    /* WHEN  */
    OTHERWISE = 266,               /* OTHERWISE  */
    REPEAT = 267,                  /* REPEAT  */
    LOOP = 268,                    /* LOOP  */
    DORETURN = 269,                /* DORETURN  */
    SHOW = 270,                    /* SHOW  */
    TAKE = 271,                    /* TAKE  */
    TYPE_NUMBER = 272,             /* TYPE_NUMBER  */
    TYPE_DECIMAL = 273,            /* TYPE_DECIMAL  */
    TYPE_LETTER = 274,             /* TYPE_LETTER  */
    YES = 275,                     /* YES  */
    NO = 276,                      /* NO  */
    NOTHING = 277,                 /* NOTHING  */
    STOP = 278,                    /* STOP  */
    SKIP = 279,                    /* SKIP  */
    DEFINE = 280,                  /* DEFINE  */
    MAIN = 281,                    /* MAIN  */
    EQ = 282,                      /* EQ  */
    NE = 283,                      /* NE  */
    LE = 284,                      /* LE  */
    GE = 285,                      /* GE  */
    LT = 286,                      /* LT  */
    GT = 287,                      /* GT  */
    AND = 288,                     /* AND  */
    OR = 289,                      /* OR  */
    NOT = 290,                     /* NOT  */
    LBRACKET = 291,                /* LBRACKET  */
    RBRACKET = 292,                /* RBRACKET  */
    COMMA = 293,                   /* COMMA  */
    PLUS = 294,                    /* PLUS  */
    MINUS = 295,                   /* MINUS  */
    MUL = 296,                     /* MUL  */
    DIV = 297,                     /* DIV  */
    MOD = 298,                     /* MOD  */
    ASSIGN = 299,                  /* ASSIGN  */
    LPAREN = 300,                  /* LPAREN  */
    RPAREN = 301,                  /* RPAREN  */
    SEMICOLON = 302                /* SEMICOLON  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 11 "mpp.y"

    int intval;
    float floatval;
    char *strval;

#line 117 "mpp.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_MPP_TAB_H_INCLUDED  */
