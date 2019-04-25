/* A Bison parser, made by GNU Bison 3.3.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_HYY_HYP_Y_H_INCLUDED
# define YY_HYY_HYP_Y_H_INCLUDED
/* Debug traces.  */
#ifndef HYYDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define HYYDEBUG 1
#  else
#   define HYYDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define HYYDEBUG 1
# endif /* ! defined YYDEBUG */
#endif  /* ! defined HYYDEBUG */
#if HYYDEBUG
extern int hyydebug;
#endif
/* "%code requires" blocks.  */
#line 21 "hyp_y.y" /* yacc.c:1921  */

#include "parser.h"

#line 60 "hyp_y.h" /* yacc.c:1921  */

/* Token type.  */
#ifndef HYYTOKENTYPE
# define HYYTOKENTYPE
  enum hyytokentype
  {
    H_BOARD_FILE = 258,
    H_VERSION = 259,
    H_DATA_MODE = 260,
    H_UNITS = 261,
    H_PLANE_SEP = 262,
    H_BOARD = 263,
    H_STACKUP = 264,
    H_DEVICES = 265,
    H_SUPPLIES = 266,
    H_PAD = 267,
    H_PADSTACK = 268,
    H_NET = 269,
    H_NET_CLASS = 270,
    H_END = 271,
    H_KEY = 272,
    H_A = 273,
    H_ARC = 274,
    H_COPPER = 275,
    H_CURVE = 276,
    H_DETAILED = 277,
    H_DIELECTRIC = 278,
    H_ENGLISH = 279,
    H_LENGTH = 280,
    H_LINE = 281,
    H_METRIC = 282,
    H_N = 283,
    H_OPTIONS = 284,
    H_PERIMETER_ARC = 285,
    H_PERIMETER_SEGMENT = 286,
    H_PIN = 287,
    H_PLANE = 288,
    H_POLYGON = 289,
    H_POLYLINE = 290,
    H_POLYVOID = 291,
    H_POUR = 292,
    H_S = 293,
    H_SEG = 294,
    H_SIGNAL = 295,
    H_SIMPLIFIED = 296,
    H_SIM_BOTH = 297,
    H_SIM_IN = 298,
    H_SIM_OUT = 299,
    H_USEG = 300,
    H_VIA = 301,
    H_WEIGHT = 302,
    H_A1 = 303,
    H_A2 = 304,
    H_BR = 305,
    H_C = 306,
    H_C_QM = 307,
    H_CO_QM = 308,
    H_D = 309,
    H_ER = 310,
    H_F = 311,
    H_ID = 312,
    H_L = 313,
    H_L1 = 314,
    H_L2 = 315,
    H_LPS = 316,
    H_LT = 317,
    H_M = 318,
    H_NAME = 319,
    H_P = 320,
    H_PKG = 321,
    H_PR_QM = 322,
    H_PS = 323,
    H_R = 324,
    H_REF = 325,
    H_SX = 326,
    H_SY = 327,
    H_S1 = 328,
    H_S1X = 329,
    H_S1Y = 330,
    H_S2 = 331,
    H_S2X = 332,
    H_S2Y = 333,
    H_T = 334,
    H_TC = 335,
    H_USE_DIE_FOR_METAL = 336,
    H_V = 337,
    H_V_QM = 338,
    H_VAL = 339,
    H_W = 340,
    H_X = 341,
    H_X1 = 342,
    H_X2 = 343,
    H_XC = 344,
    H_Y = 345,
    H_Y1 = 346,
    H_Y2 = 347,
    H_YC = 348,
    H_Z = 349,
    H_ZL = 350,
    H_ZLEN = 351,
    H_ZW = 352,
    H_YES = 353,
    H_NO = 354,
    H_BOOL = 355,
    H_POSINT = 356,
    H_FLOAT = 357,
    H_STRING = 358
  };
#endif

/* Value type.  */
#if ! defined HYYSTYPE && ! defined HYYSTYPE_IS_DECLARED

union HYYSTYPE
{
#line 30 "hyp_y.y" /* yacc.c:1921  */

    int boolval;
    int intval;
    double floatval;
    char* strval;

#line 183 "hyp_y.h" /* yacc.c:1921  */
};

typedef union HYYSTYPE HYYSTYPE;
# define HYYSTYPE_IS_TRIVIAL 1
# define HYYSTYPE_IS_DECLARED 1
#endif


extern HYYSTYPE hyylval;

int hyyparse (void);

#endif /* !YY_HYY_HYP_Y_H_INCLUDED  */
