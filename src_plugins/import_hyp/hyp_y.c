/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE         HYYSTYPE
/* Substitute the variable and function names.  */
#define yyparse         hyyparse
#define yylex           hyylex
#define yyerror         hyyerror
#define yydebug         hyydebug
#define yynerrs         hyynerrs

#define yylval          hyylval
#define yychar          hyychar

/* Copy the first part of user declarations.  */

#line 76 "hyp_y.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "hyp_y.h".  */
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
#line 21 "hyp_y.y" /* yacc.c:355  */

#include "parser.h"

#line 118 "hyp_y.c" /* yacc.c:355  */

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
#line 30 "hyp_y.y" /* yacc.c:355  */

    int boolval;
    int intval;
    double floatval;
    char* strval;

#line 241 "hyp_y.c" /* yacc.c:355  */
};

typedef union HYYSTYPE HYYSTYPE;
# define HYYSTYPE_IS_TRIVIAL 1
# define HYYSTYPE_IS_DECLARED 1
#endif


extern HYYSTYPE hyylval;

int hyyparse (void);

#endif /* !YY_HYY_HYP_Y_H_INCLUDED  */

/* Copy the second part of user declarations.  */
#line 37 "hyp_y.y" /* yacc.c:358  */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void hyyerror(const char *);

/* HYYPRINT and hyyprint print values of the tokens when debugging is switched on */
void hyyprint(FILE *, int, HYYSTYPE);
#define HYYPRINT(file, type, value) hyyprint (file, type, value)

/* clear parse_param struct at beginning of new record */
void new_record();

/* struct to pass to calling class */
parse_param h;


#line 277 "hyp_y.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
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


#if ! defined yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined HYYSTYPE_IS_TRIVIAL && HYYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
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
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  33
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   759

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  110
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  179
/* YYNRULES -- Number of rules.  */
#define YYNRULES  309
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  617

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   358

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     107,   108,     2,     2,   109,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   106,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   104,     2,   105,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103
};

#if HYYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   127,   127,   128,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   150,
     150,   155,   155,   160,   163,   164,   169,   172,   173,   176,
     177,   181,   181,   186,   187,   190,   191,   194,   195,   199,
     200,   201,   202,   205,   208,   211,   211,   211,   216,   219,
     220,   223,   224,   225,   226,   227,   230,   233,   233,   234,
     238,   238,   241,   242,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   257,   257,   260,   261,   264,   265,
     266,   267,   268,   269,   270,   271,   275,   275,   278,   279,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   293,
     296,   299,   302,   305,   308,   311,   314,   317,   320,   323,
     328,   329,   332,   333,   336,   336,   336,   336,   337,   340,
     341,   345,   346,   350,   351,   355,   358,   359,   363,   366,
     369,   374,   377,   378,   381,   382,   385,   388,   393,   393,
     393,   396,   396,   397,   398,   401,   402,   405,   405,   406,
     409,   409,   410,   414,   414,   414,   417,   418,   419,   420,
     421,   422,   423,   420,   430,   430,   433,   433,   434,   437,
     438,   438,   442,   443,   446,   447,   448,   449,   450,   451,
     452,   453,   454,   455,   456,   457,   461,   461,   464,   464,
     467,   468,   472,   473,   477,   480,   483,   483,   487,   488,
     492,   494,   495,   496,   497,   498,   499,   500,   501,   502,
     503,   504,   505,   506,   507,   508,   512,   515,   518,   521,
     521,   524,   525,   529,   530,   534,   537,   538,   539,   543,
     543,   546,   547,   551,   552,   553,   554,   555,   559,   559,
     562,   563,   567,   568,   569,   567,   574,   575,   574,   579,
     579,   581,   585,   585,   585,   589,   590,   594,   595,   596,
     597,   601,   605,   606,   607,   611,   611,   611,   615,   615,
     615,   619,   620,   624,   625,   626,   630,   630,   633,   633,
     636,   636,   636,   641,   641,   644,   645,   649,   650,   654,
     655,   656,   660,   660,   663,   663,   663,   668,   673,   673,
     678,   678,   681,   681,   684,   684,   687,   690,   690,   690
};
#endif

#if HYYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "H_BOARD_FILE", "H_VERSION",
  "H_DATA_MODE", "H_UNITS", "H_PLANE_SEP", "H_BOARD", "H_STACKUP",
  "H_DEVICES", "H_SUPPLIES", "H_PAD", "H_PADSTACK", "H_NET", "H_NET_CLASS",
  "H_END", "H_KEY", "H_A", "H_ARC", "H_COPPER", "H_CURVE", "H_DETAILED",
  "H_DIELECTRIC", "H_ENGLISH", "H_LENGTH", "H_LINE", "H_METRIC", "H_N",
  "H_OPTIONS", "H_PERIMETER_ARC", "H_PERIMETER_SEGMENT", "H_PIN",
  "H_PLANE", "H_POLYGON", "H_POLYLINE", "H_POLYVOID", "H_POUR", "H_S",
  "H_SEG", "H_SIGNAL", "H_SIMPLIFIED", "H_SIM_BOTH", "H_SIM_IN",
  "H_SIM_OUT", "H_USEG", "H_VIA", "H_WEIGHT", "H_A1", "H_A2", "H_BR",
  "H_C", "H_C_QM", "H_CO_QM", "H_D", "H_ER", "H_F", "H_ID", "H_L", "H_L1",
  "H_L2", "H_LPS", "H_LT", "H_M", "H_NAME", "H_P", "H_PKG", "H_PR_QM",
  "H_PS", "H_R", "H_REF", "H_SX", "H_SY", "H_S1", "H_S1X", "H_S1Y", "H_S2",
  "H_S2X", "H_S2Y", "H_T", "H_TC", "H_USE_DIE_FOR_METAL", "H_V", "H_V_QM",
  "H_VAL", "H_W", "H_X", "H_X1", "H_X2", "H_XC", "H_Y", "H_Y1", "H_Y2",
  "H_YC", "H_Z", "H_ZL", "H_ZLEN", "H_ZW", "H_YES", "H_NO", "H_BOOL",
  "H_POSINT", "H_FLOAT", "H_STRING", "'{'", "'}'", "'='", "'('", "')'",
  "','", "$accept", "hyp_file", "hyp_section", "board_file", "$@1",
  "version", "$@2", "data_mode", "mode", "units", "unit_system",
  "metal_thickness_unit", "plane_sep", "$@3", "board", "board_param_list",
  "board_param_list_item", "board_param", "perimeter_segment",
  "perimeter_arc", "board_attribute", "$@4", "$@5", "stackup",
  "stackup_paramlist", "stackup_param", "options", "options_params", "$@6",
  "signal", "$@7", "signal_paramlist", "signal_param", "dielectric", "$@8",
  "dielectric_paramlist", "dielectric_param", "plane", "$@9",
  "plane_paramlist", "plane_param", "thickness", "plating_thickness",
  "bulk_resistivity", "temperature_coefficient", "epsilon_r",
  "loss_tangent", "layer_name", "material_name", "plane_separation",
  "conformal", "prepreg", "devices", "device_list", "device", "$@10",
  "$@11", "$@12", "device_paramlist", "device_value", "device_layer",
  "name", "value", "value_float", "value_string", "package", "supplies",
  "supply_list", "supply", "voltage_spec", "conversion", "padstack",
  "$@13", "$@14", "drill_size", "$@15", "padstack_list", "padstack_def",
  "$@16", "pad_shape", "$@17", "pad_coord", "$@18", "$@19", "pad_type",
  "$@20", "$@21", "$@22", "$@23", "net", "$@24", "net_separation", "$@25",
  "net_copper", "$@26", "net_subrecord_list", "net_subrecord", "seg",
  "$@27", "arc", "$@28", "ps_lps_param", "lps_param", "width",
  "left_plane_separation", "via", "$@29", "via_param_list", "via_param",
  "padstack_name", "layer1_name", "layer2_name", "pin", "$@30",
  "pin_param", "pin_function_param", "pin_reference", "pin_function",
  "pad", "$@31", "pad_param_list", "pad_param", "useg", "$@32",
  "useg_param", "useg_stackup", "$@33", "$@34", "$@35", "useg_impedance",
  "$@36", "$@37", "useg_resistance", "$@38", "polygon", "$@39", "$@40",
  "polygon_param_list", "polygon_param", "polygon_id", "polygon_type",
  "polyvoid", "$@41", "$@42", "polyline", "$@43", "$@44",
  "lines_and_curves", "line_or_curve", "line", "$@45", "curve", "$@46",
  "net_attribute", "$@47", "$@48", "netclass", "$@49",
  "netclass_subrecords", "netclass_paramlist", "netclass_param",
  "net_name", "$@50", "netclass_attribute", "$@51", "$@52", "end", "key",
  "$@53", "coord_point", "$@54", "coord_point1", "$@55", "coord_point2",
  "$@56", "coord_line", "coord_arc", "$@57", "$@58", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   123,   125,    61,    40,    41,    44
};
# endif

#define YYPACT_NINF -329

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-329)))

#define YYTABLE_NINF -115

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -61,   278,    10,  -329,  -329,  -329,  -329,  -329,  -329,  -329,
    -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,   -84,  -329,
     -55,   -22,   -16,     1,    -5,    44,    79,    49,  -329,    66,
      92,    87,   117,  -329,  -329,  -329,   109,   125,    19,   108,
     129,  -329,   179,    86,  -329,   156,   158,  -329,  -329,  -329,
    -329,  -329,  -329,     8,   197,  -329,    28,   198,  -329,   140,
     132,   150,  -329,   171,  -329,  -329,  -329,  -329,   206,  -329,
    -329,    -7,  -329,  -329,   239,   203,   203,   193,  -329,  -329,
    -329,  -329,  -329,   211,  -329,    13,  -329,  -329,  -329,  -329,
     214,   220,  -329,  -329,   216,   261,  -329,  -329,   223,  -329,
    -329,  -329,   222,  -329,  -329,  -329,   224,   225,   226,   227,
     240,   242,  -329,  -329,  -329,  -329,   185,   228,  -329,  -329,
     162,   153,  -329,  -329,  -329,   229,   252,  -329,    95,   201,
     232,  -329,  -329,  -329,   235,   237,   234,  -329,   236,   238,
     241,   243,   244,   245,   246,   247,   248,   111,  -329,  -329,
    -329,  -329,  -329,  -329,  -329,  -329,   255,   250,   251,   253,
     254,    97,  -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,
    -329,   257,   258,    30,  -329,  -329,  -329,  -329,  -329,  -329,
    -329,  -329,  -329,  -329,   271,   259,   260,   262,    11,    90,
     149,  -329,  -329,  -329,   263,   164,  -329,  -329,  -329,  -329,
    -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,   233,  -329,
     202,  -329,  -329,  -329,  -329,  -329,  -329,   265,   267,   268,
     272,   269,   270,   273,   274,   276,   277,  -329,  -329,  -329,
     279,   280,   281,   282,  -329,  -329,   283,   284,  -329,  -329,
     285,  -329,   286,   287,   291,    12,   -71,   275,   288,  -329,
     289,  -329,  -329,  -329,   266,  -329,   318,  -329,  -329,  -329,
    -329,  -329,   169,  -329,  -329,  -329,   290,   320,   330,  -329,
    -329,   296,   298,  -329,  -329,  -329,  -329,  -329,  -329,  -329,
    -329,  -329,  -329,   292,  -329,  -329,  -329,  -329,  -329,  -329,
     293,  -329,   297,   295,   294,   299,  -329,  -329,   288,  -329,
    -329,  -329,   187,   187,   304,  -329,   301,   300,   203,   301,
     203,   203,   301,  -329,  -329,   302,   303,   305,   306,   307,
     310,  -329,  -329,  -329,   313,  -329,  -329,   308,   288,   309,
     312,   314,  -329,  -329,   143,  -329,  -329,  -329,   143,   301,
     315,    65,   311,   319,   321,   319,   333,    68,   316,   322,
     323,   325,   317,   324,   196,  -329,   -83,   288,   264,   138,
     326,  -329,  -329,  -329,  -329,   327,   328,   329,   331,   332,
    -329,     4,  -329,  -329,   347,   334,   -26,   347,   335,   240,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   348,   349,   350,   351,    -4,  -329,  -329,  -329,  -329,
    -329,  -329,  -329,  -329,   356,   357,   354,   358,   353,  -329,
    -329,    50,   347,  -329,  -329,  -329,  -329,   360,  -329,  -329,
    -329,  -329,  -329,   359,   359,   359,  -329,   361,   362,   365,
     366,  -329,  -329,   371,   -45,   367,   363,  -329,   -21,  -329,
    -329,   364,   -45,   368,   372,   373,   370,   374,   375,   376,
     377,   378,   380,   381,   382,   384,   385,   386,   388,   389,
    -329,  -329,   392,   387,  -329,  -329,  -329,   -75,   390,  -329,
    -329,  -329,  -329,   355,  -329,    98,   249,   205,  -329,  -329,
    -329,   209,   210,   394,  -329,  -329,  -329,  -329,   391,   393,
    -329,   -44,  -329,  -329,   395,  -329,   256,  -329,  -329,  -329,
    -329,   163,  -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,
    -329,  -329,  -329,  -329,  -329,  -329,  -329,   396,  -329,   410,
    -329,   397,  -329,   379,  -329,    -6,  -329,   398,  -329,  -329,
    -329,  -329,  -329,  -329,   399,   401,   405,  -329,  -329,  -329,
    -329,  -329,   402,   403,  -329,  -329,  -329,   407,   406,  -329,
     409,   408,   411,  -329,  -329,   203,   301,   412,  -329,  -329,
     413,   414,  -329,   416,  -329,  -329,  -329,   383,   415,   417,
    -329,   418,  -329,  -329,   419,  -329,   404,   420,  -329,  -329,
    -329,   439,   423,  -329,   422,  -329,   424,   425,  -329,   426,
     427,   430,   431,  -329,  -329,  -329,   -49,   432,   428,   433,
    -329,  -329,   434,   436,   440,   441,  -329,  -329,  -329,   435,
     437,   438,   352,  -329,  -329,   442,  -329
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,     0,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,     0,    19,
       0,     0,     0,     0,     0,     0,     0,     0,   138,     0,
       0,     0,     0,     1,     2,    18,     0,     0,     0,     0,
       0,    34,     0,     0,    36,     0,     0,    50,    51,    52,
      53,    54,   111,     0,     0,   113,     0,     0,   133,     0,
       0,     0,   297,     0,    20,    21,    25,    24,     0,    27,
      28,     0,    31,    42,     0,     0,     0,    38,    39,    40,
      41,    33,    35,     0,    74,     0,    86,    60,    48,    49,
       0,     0,   110,   112,     0,     0,   131,   132,     0,   164,
     283,   298,     0,    23,    30,    29,     0,     0,     0,     0,
       0,     0,    44,    43,    37,    55,     0,     0,    59,    56,
       0,     0,   118,   115,   135,     0,     0,   139,   170,     0,
       0,    22,    26,    32,     0,     0,     0,   306,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    77,    78,
      80,    81,    84,    85,    82,    83,     0,     0,     0,     0,
       0,     0,    89,    90,    92,    93,    94,    95,    96,    97,
      98,     0,     0,     0,    63,    64,    65,    67,    68,    69,
      70,    71,    72,    73,     0,     0,     0,     0,     0,     0,
       0,   166,   165,   168,     0,     0,   173,   174,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   286,     0,   284,
       0,   288,   290,   289,   299,    45,   302,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    75,    76,    57,
       0,     0,     0,     0,    87,    88,     0,     0,    61,    62,
       0,   125,     0,     0,     0,     0,     0,     0,   144,   146,
       0,   252,   268,   265,     0,   229,     0,   188,   219,   186,
     238,   196,   170,   171,   169,   172,     0,     0,     0,   285,
     287,     0,     0,   304,   307,    79,   108,   103,   105,   104,
     106,   109,    99,     0,   101,    91,   107,   102,    66,   100,
       0,   128,     0,     0,     0,     0,   147,   141,   143,   140,
     145,   185,     0,     0,     0,   184,     0,     0,     0,     0,
       0,     0,     0,   167,   291,     0,     0,     0,     0,     0,
       0,    58,   116,   136,     0,   134,   149,     0,     0,     0,
       0,     0,   257,   258,     0,   256,   260,   259,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   137,     0,   142,     0,     0,
       0,   255,   253,   269,   266,     0,     0,     0,     0,     0,
     233,     0,   232,   280,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   199,   200,   202,   203,
     294,   292,    46,   303,     0,     0,     0,   124,     0,   120,
     122,     0,     0,   126,   127,   150,   152,     0,   261,   264,
     263,   262,   194,     0,     0,     0,   300,     0,     0,     0,
       0,   230,   231,     0,     0,     0,     0,   224,     0,   220,
     222,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     197,   198,     0,     0,    47,   305,   308,     0,     0,   123,
     117,   119,   121,     0,   153,     0,     0,     0,   272,   273,
     274,     0,     0,     0,   237,   234,   235,   236,     0,     0,
     193,     0,   189,   191,     0,   225,     0,   221,   223,   187,
     217,     0,   207,   204,   211,   215,   201,   218,   216,   205,
     206,   208,   209,   210,   212,   213,   214,     0,   293,     0,
     129,     0,   151,     0,   156,     0,   148,     0,   278,   276,
     254,   271,   270,   267,     0,     0,     0,   190,   192,   228,
     227,   226,     0,     0,   239,   240,   241,     0,     0,   130,
       0,     0,     0,   159,   275,     0,     0,     0,   281,   195,
       0,     0,   295,     0,   154,   158,   157,     0,     0,     0,
     301,     0,   246,   242,     0,   309,     0,     0,   279,   277,
     282,     0,     0,   296,     0,   160,     0,     0,   155,     0,
       0,     0,     0,   247,   243,   161,     0,     0,     0,     0,
     251,   248,     0,     0,     0,     0,   162,   249,   244,     0,
       0,     0,     0,   250,   245,     0,   163
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -329,  -329,   492,  -329,  -329,  -329,  -329,  -329,  -329,  -329,
    -329,  -329,  -329,  -329,  -329,  -329,   455,  -329,  -329,  -329,
    -329,  -329,  -329,  -329,  -329,   475,  -329,  -329,  -329,  -329,
    -329,  -329,   400,  -329,  -329,  -329,   421,  -329,  -329,  -329,
     429,   -96,  -329,   -74,   -68,   -72,   -42,  -115,     9,  -121,
    -329,  -329,  -329,  -329,   447,  -329,  -329,  -329,  -329,    -1,
      21,    62,  -329,   443,  -329,  -329,  -329,  -329,   477,  -329,
    -329,  -329,  -329,  -329,  -329,  -329,  -231,  -244,  -329,  -329,
    -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,
    -329,  -329,  -329,   160,  -329,  -329,   369,  -329,  -329,  -329,
    -329,    -3,    45,   -25,  -329,  -329,  -329,  -329,    41,   165,
     207,   103,  -329,  -329,  -329,   110,  -329,  -329,  -329,  -329,
    -329,   178,  -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,
    -329,  -329,  -329,  -329,  -329,  -329,  -329,   444,  -151,   445,
    -329,  -329,  -329,  -329,  -329,  -329,  -329,  -147,  -328,  -329,
    -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,
     446,  -329,  -329,  -329,  -329,  -329,  -329,  -329,  -329,  -301,
    -329,   448,  -329,   172,  -329,   -73,  -306,  -329,  -329
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     4,    36,     5,   102,     6,    68,     7,
      71,   106,     8,   107,     9,    43,    44,    77,    78,    79,
      80,   271,   464,    10,    46,    47,    48,   119,   283,    49,
     121,   173,   174,    50,   116,   147,   148,    51,   120,   161,
     162,   149,   176,   164,   165,   150,   151,   332,   153,   170,
     154,   155,    11,    54,    55,    91,   184,   354,   408,   409,
     410,   126,   412,   413,   414,   469,    12,    57,    58,   244,
     294,    13,    59,   188,   247,   328,   248,   249,   327,   417,
     473,   475,   523,   576,   526,   567,   589,   598,   609,    14,
     128,   192,   262,   193,   194,   195,   196,   197,   310,   198,
     308,   492,   493,   333,   494,   199,   312,   395,   396,   397,
     398,   399,   200,   309,   439,   440,   376,   441,   201,   306,
     371,   372,   202,   311,   544,   545,   582,   597,   611,   546,
     581,   596,   601,   610,   203,   302,   423,   334,   335,   336,
     337,   204,   304,   425,   205,   303,   424,   477,   478,   479,
     556,   480,   555,   206,   433,   571,    15,   129,   209,   210,
     211,   212,   463,   213,   462,   574,    16,    17,   130,   341,
     483,   110,   272,   137,   319,   111,   112,   320,   519
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     183,   152,   343,   113,   300,   168,   181,   191,   344,    90,
      33,   347,   551,   295,   380,   298,   489,   489,   104,   415,
     599,    35,   366,   159,   163,   175,   416,   291,   520,    94,
     436,   297,   152,   362,   381,   436,   245,   363,   364,   386,
     105,    66,   367,     1,   382,   383,   168,   177,   166,   179,
     384,    37,   183,   178,   300,   378,   385,   552,   181,   600,
      67,   386,   142,   490,   490,   163,    95,   387,   388,   389,
     390,   391,   392,   393,   394,   368,   369,   175,   167,   180,
     157,   171,   437,   366,    38,   141,   380,   437,   142,   166,
      39,   250,   143,   144,   117,   172,   553,   357,   159,   177,
      41,   179,    42,   367,   460,   178,   381,    40,   142,   146,
     160,  -114,   431,   300,     1,   296,   382,   383,   245,   167,
     246,   118,   384,   142,   251,   252,   253,   378,   385,   169,
     182,   180,    69,   386,   406,    70,   368,   369,   238,   387,
     388,   389,   390,   391,   392,   393,   394,   157,   158,   531,
     254,    45,   141,   531,   531,   142,    56,    83,   419,   143,
     144,   255,   139,   159,   140,   159,   141,   256,   257,   142,
     169,   420,    60,   143,   144,   421,   146,   160,   145,    84,
      73,   258,   182,   361,    52,    85,    53,   361,   259,    86,
     146,    81,    62,    42,   260,   261,    87,    74,    61,   189,
     329,   142,   190,   157,   171,   234,   524,   525,   141,    75,
      76,   142,   157,   158,    64,   143,   144,   141,   172,   227,
     142,   159,   330,    63,   143,   144,   370,    65,   331,   340,
     159,    72,   146,   160,   266,    99,   139,   345,   140,   407,
     141,   146,   160,   142,   329,   142,    98,   143,   144,   568,
     527,   267,   145,   100,   142,   569,   370,   542,   543,   434,
     125,   268,   442,    88,   146,    45,   330,   108,   189,   264,
     528,   190,   331,   189,   101,   529,   190,   481,   482,    18,
     406,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     109,    28,    29,    30,    31,    32,   407,   407,   539,   540,
     541,   114,    92,    96,    53,    56,   207,   269,   208,   208,
     530,   103,   476,   491,   532,   533,   476,   476,   374,   115,
     377,   491,   122,   123,   124,   125,   127,   131,   136,   132,
     133,   138,   134,   135,   156,   185,   186,   214,   215,   216,
     217,   240,   218,   293,   219,   243,   307,   220,   315,   221,
     222,   223,   224,   225,   226,   229,   230,   231,   316,   232,
     233,   329,   241,   236,   237,   418,   242,   273,   263,   274,
     275,   277,   276,   278,   305,   279,   281,   280,   317,   282,
     299,   284,   285,   286,   287,   288,   289,   340,   291,   318,
     375,   290,   378,   292,   301,   245,   322,   323,   314,   352,
     321,   324,   325,   353,   331,   142,   342,   326,   348,   349,
     471,   350,   351,   355,   373,   358,   411,   356,   359,   400,
     360,   365,   313,   404,   468,   401,   402,   403,   422,   426,
     405,   615,   385,   472,   427,   428,   461,   429,   430,   499,
     435,   443,   445,   446,   447,   448,   449,   450,   451,   452,
     453,   454,   455,   488,   456,   457,   458,   459,   465,   466,
     467,   470,   474,   484,   522,   485,   476,   486,   487,   496,
     495,   500,   498,   503,   517,   502,   504,   505,   506,   548,
     507,   508,   509,   510,   534,   511,   512,   513,   550,   514,
     515,   516,   577,   586,    34,   518,   521,   535,    82,   536,
     549,    93,   547,   538,   558,   557,   554,   559,   560,   561,
     562,   564,   563,   584,   570,   572,   565,   573,   575,   566,
     587,    89,   585,   578,   588,   579,   580,   583,   602,   593,
     590,   591,   594,   595,    97,   592,   537,   603,   606,   604,
     605,   438,   607,   608,   612,   613,   614,   501,   497,   432,
     616,   444,     0,   379,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   265,     0,     0,     0,   228,   187,
       0,     0,     0,   239,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     235,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   270,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   338,     0,   339,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   346
};

static const yytype_int16 yycheck[] =
{
     121,   116,   308,    76,   248,   120,   121,   128,   309,     1,
       0,   312,    18,     1,    18,   246,    61,    61,    25,   102,
      69,   105,    18,    68,   120,   121,   109,   102,   103,     1,
      56,   102,   147,   334,    38,    56,   107,   338,   339,    65,
      47,    22,    38,   104,    48,    49,   161,   121,   120,   121,
      54,   106,   173,   121,   298,    59,    60,    63,   173,   108,
      41,    65,    58,   108,   108,   161,    38,    71,    72,    73,
      74,    75,    76,    77,    78,    71,    72,   173,   120,   121,
      50,    51,   108,    18,   106,    55,    18,   108,    58,   161,
     106,     1,    62,    63,    81,    65,   102,   328,    68,   173,
     105,   173,   107,    38,   108,   173,    38,   106,    58,    79,
      80,   103,   108,   357,   104,   103,    48,    49,   107,   161,
     109,   108,    54,    58,    34,    35,    36,    59,    60,   120,
     121,   173,    24,    65,    84,    27,    71,    72,   108,    71,
      72,    73,    74,    75,    76,    77,    78,    50,    51,   477,
       1,   107,    55,   481,   482,    58,   107,     1,    20,    62,
      63,    12,    51,    68,    53,    68,    55,    18,    19,    58,
     161,    33,   106,    62,    63,    37,    79,    80,    67,    23,
       1,    32,   173,   334,   105,    29,   107,   338,    39,    33,
      79,   105,   105,   107,    45,    46,    40,    18,   106,   104,
      57,    58,   107,    50,    51,   108,   108,   109,    55,    30,
      31,    58,    50,    51,   105,    62,    63,    55,    65,   108,
      58,    68,    79,   106,    62,    63,   341,   102,    85,    86,
      68,   102,    79,    80,     1,   103,    51,   310,    53,   354,
      55,    79,    80,    58,    57,    58,   106,    62,    63,   555,
       1,    18,    67,   103,    58,   556,   371,    94,    95,   374,
      64,    28,   377,   105,    79,   107,    79,    28,   104,   105,
      21,   107,    85,   104,   103,    26,   107,   424,   425,     1,
      84,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      87,    13,    14,    15,    16,    17,   411,   412,    42,    43,
      44,   108,   105,   105,   107,   107,   105,   105,   107,   107,
     105,   105,   107,   434,   105,   105,   107,   107,   343,   108,
     345,   442,   108,   103,   108,    64,   103,   105,    88,   105,
     105,    89,   106,   106,   106,   106,    84,   105,   103,   102,
     106,    70,   106,    52,   106,    83,    28,   106,    28,   106,
     106,   106,   106,   106,   106,   100,   106,   106,    28,   106,
     106,    57,   103,   106,   106,   101,   106,   102,   105,   102,
     102,   102,   100,   103,   108,   102,   100,   103,    82,   102,
     105,   102,   102,   102,   102,   102,   102,    86,   102,    91,
      69,   106,    59,   106,   105,   107,   103,   100,   108,    92,
     108,   106,   108,    93,    85,    58,   106,   108,   106,   106,
     411,   106,   106,   100,   103,   106,   354,   109,   106,   103,
     106,   106,   262,   106,    66,   103,   103,   102,   102,   102,
     106,    79,    60,   412,   106,   106,   395,   106,   106,   442,
     106,   106,   106,   106,   106,   106,   106,   106,   106,   106,
     106,   106,   106,    82,   106,   106,   106,   106,   102,   102,
     106,   108,   102,   102,   109,   103,   107,   102,   102,   106,
     103,   103,   108,   103,    82,   102,   102,   102,   102,    69,
     103,   103,   102,   102,    90,   103,   102,   102,   109,   103,
     102,   102,   109,    54,     2,   108,   106,   106,    43,   106,
     103,    54,   106,   108,   103,   106,   108,   102,   106,   106,
     103,   102,   106,   109,   102,   102,   108,   103,   102,   108,
      97,    46,   102,   108,   102,   108,   108,   108,    96,   102,
     106,   106,   102,   102,    57,   109,   491,   109,   102,   106,
     106,   376,   102,   102,   109,   108,   108,   444,   438,   371,
     108,   379,    -1,   346,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   195,    -1,    -1,    -1,   147,   126,
      -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     161,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   210,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   303,    -1,   304,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   311
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   104,   111,   112,   113,   115,   117,   119,   122,   124,
     133,   162,   176,   181,   199,   266,   276,   277,     1,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    13,    14,
      15,    16,    17,     0,   112,   105,   114,   106,   106,   106,
     106,   105,   107,   125,   126,   107,   134,   135,   136,   139,
     143,   147,   105,   107,   163,   164,   107,   177,   178,   182,
     106,   106,   105,   106,   105,   102,    22,    41,   118,    24,
      27,   120,   102,     1,    18,    30,    31,   127,   128,   129,
     130,   105,   126,     1,    23,    29,    33,    40,   105,   135,
       1,   165,   105,   164,     1,    38,   105,   178,   106,   103,
     103,   103,   116,   105,    25,    47,   121,   123,    28,    87,
     281,   285,   286,   285,   108,   108,   144,    81,   108,   137,
     148,   140,   108,   103,   108,    64,   171,   103,   200,   267,
     278,   105,   105,   105,   106,   106,    88,   283,    89,    51,
      53,    55,    58,    62,    63,    67,    79,   145,   146,   151,
     155,   156,   157,   158,   160,   161,   106,    50,    51,    68,
      80,   149,   150,   151,   153,   154,   155,   156,   157,   158,
     159,    51,    65,   141,   142,   151,   152,   153,   154,   155,
     156,   157,   158,   159,   166,   106,    84,   173,   183,   104,
     107,   159,   201,   203,   204,   205,   206,   207,   209,   215,
     222,   228,   232,   244,   251,   254,   263,   105,   107,   268,
     269,   270,   271,   273,   105,   103,   102,   106,   106,   106,
     106,   106,   106,   106,   106,   106,   106,   108,   146,   100,
     106,   106,   106,   106,   108,   150,   106,   106,   108,   142,
      70,   103,   106,    83,   179,   107,   109,   184,   186,   187,
       1,    34,    35,    36,     1,    12,    18,    19,    32,    39,
      45,    46,   202,   105,   105,   206,     1,    18,    28,   105,
     270,   131,   282,   102,   102,   102,   100,   102,   103,   102,
     103,   100,   102,   138,   102,   102,   102,   102,   102,   102,
     106,   102,   106,    52,   180,     1,   103,   102,   186,   105,
     187,   105,   245,   255,   252,   108,   229,    28,   210,   223,
     208,   233,   216,   203,   108,    28,    28,    82,    91,   284,
     287,   108,   103,   100,   106,   108,   108,   188,   185,    57,
      79,    85,   157,   213,   247,   248,   249,   250,   247,   249,
      86,   279,   106,   286,   279,   285,   281,   279,   106,   106,
     106,   106,    92,    93,   167,   100,   109,   186,   106,   106,
     106,   248,   279,   279,   279,   106,    18,    38,    71,    72,
     157,   230,   231,   103,   213,    69,   226,   213,    59,   220,
      18,    38,    48,    49,    54,    60,    65,    71,    72,    73,
      74,    75,    76,    77,    78,   217,   218,   219,   220,   221,
     103,   103,   103,   102,   106,   106,    84,   157,   168,   169,
     170,   171,   172,   173,   174,   102,   109,   189,   101,    20,
      33,    37,   102,   246,   256,   253,   102,   106,   106,   106,
     106,   108,   231,   264,   157,   106,    56,   108,   219,   224,
     225,   227,   157,   106,   283,   106,   106,   106,   106,   106,
     106,   106,   106,   106,   106,   106,   106,   106,   106,   106,
     108,   218,   274,   272,   132,   102,   102,   106,    66,   175,
     108,   169,   170,   190,   102,   191,   107,   257,   258,   259,
     261,   257,   257,   280,   102,   103,   102,   102,    82,    61,
     108,   159,   211,   212,   214,   103,   106,   225,   108,   211,
     103,   221,   102,   103,   102,   102,   102,   103,   103,   102,
     102,   103,   102,   102,   103,   102,   102,    82,   108,   288,
     103,   106,   109,   192,   108,   109,   194,     1,    21,    26,
     105,   258,   105,   105,    90,   106,   106,   212,   108,    42,
      43,    44,    94,    95,   234,   235,   239,   106,    69,   103,
     109,    18,    63,   102,   108,   262,   260,   106,   103,   102,
     106,   106,   103,   106,   102,   108,   108,   195,   286,   279,
     102,   265,   102,   103,   275,   102,   193,   109,   108,   108,
     108,   240,   236,   108,   109,   102,    54,    97,   102,   196,
     106,   106,   109,   102,   102,   102,   241,   237,   197,    69,
     108,   242,    96,   109,   106,   106,   102,   102,   102,   198,
     243,   238,   109,   108,   108,    79,   108
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   110,   111,   111,   112,   112,   112,   112,   112,   112,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   114,
     113,   116,   115,   117,   118,   118,   119,   120,   120,   121,
     121,   123,   122,   124,   124,   125,   125,   126,   126,   127,
     127,   127,   127,   128,   129,   131,   132,   130,   133,   134,
     134,   135,   135,   135,   135,   135,   136,   138,   137,   137,
     140,   139,   141,   141,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   144,   143,   145,   145,   146,   146,
     146,   146,   146,   146,   146,   146,   148,   147,   149,   149,
     150,   150,   150,   150,   150,   150,   150,   150,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   162,   163,   163,   165,   166,   167,   164,   164,   168,
     168,   169,   169,   170,   170,   171,   172,   172,   173,   174,
     175,   176,   177,   177,   178,   178,   179,   180,   182,   183,
     181,   185,   184,   184,   184,   186,   186,   188,   187,   187,
     190,   189,   189,   192,   193,   191,   194,   194,   194,   195,
     196,   197,   198,   194,   200,   199,   202,   201,   201,   203,
     204,   203,   205,   205,   206,   206,   206,   206,   206,   206,
     206,   206,   206,   206,   206,   206,   208,   207,   210,   209,
     211,   211,   212,   212,   213,   214,   216,   215,   217,   217,
     218,   218,   218,   218,   218,   218,   218,   218,   218,   218,
     218,   218,   218,   218,   218,   218,   219,   220,   221,   223,
     222,   224,   224,   225,   225,   226,   227,   227,   227,   229,
     228,   230,   230,   231,   231,   231,   231,   231,   233,   232,
     234,   234,   236,   237,   238,   235,   240,   241,   239,   243,
     242,   242,   245,   246,   244,   247,   247,   248,   248,   248,
     248,   249,   250,   250,   250,   252,   253,   251,   255,   256,
     254,   257,   257,   258,   258,   258,   260,   259,   262,   261,
     264,   265,   263,   267,   266,   268,   268,   269,   269,   270,
     270,   270,   272,   271,   274,   275,   273,   276,   278,   277,
     280,   279,   282,   281,   284,   283,   285,   287,   288,   286
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     0,
       4,     0,     6,     5,     1,     1,     6,     1,     1,     1,
       1,     0,     6,     4,     3,     2,     1,     3,     2,     1,
       1,     1,     1,     2,     2,     0,     0,     9,     4,     2,
       1,     1,     1,     1,     1,     3,     3,     0,     5,     1,
       0,     5,     2,     1,     1,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     0,     5,     2,     1,     1,     3,
       1,     1,     1,     1,     1,     1,     0,     5,     2,     1,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       4,     3,     2,     1,     0,     0,     0,    10,     3,     2,
       1,     2,     1,     2,     1,     3,     1,     1,     3,     3,
       3,     4,     2,     1,     7,     3,     3,     3,     0,     0,
       8,     0,     4,     2,     1,     2,     1,     0,     7,     3,
       0,     3,     1,     0,     0,     7,     1,     3,     3,     0,
       0,     0,     0,    15,     0,     6,     0,     3,     1,     2,
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     0,     7,     0,     7,
       2,     1,     2,     1,     3,     3,     0,     6,     2,     1,
       1,     3,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     0,
       6,     2,     1,     2,     1,     3,     3,     3,     3,     0,
       6,     2,     1,     1,     3,     3,     3,     3,     0,     8,
       1,     1,     0,     0,     0,    13,     0,     0,     9,     0,
       5,     1,     0,     0,     8,     2,     1,     1,     1,     1,
       1,     3,     3,     3,     3,     0,     0,     8,     0,     0,
       8,     2,     1,     1,     1,     3,     0,     5,     0,     5,
       0,     0,    11,     0,     6,     2,     1,     2,     1,     1,
       1,     3,     0,     7,     0,     0,    11,     3,     0,     6,
       0,     7,     0,     7,     0,     7,     2,     0,     0,    12
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if HYYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
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
#else /* !HYYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !HYYDEBUG */


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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
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
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
| yyreduce -- Do a reduction.  |
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
        case 19:
#line 150 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_board_file(&h)) YYERROR; }
#line 1867 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 21:
#line 155 "hyp_y.y" /* yacc.c:1646  */
    { h.vers = yylval.floatval; }
#line 1873 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 22:
#line 155 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_version(&h)) YYERROR; }
#line 1879 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 23:
#line 160 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_data_mode(&h)) YYERROR; }
#line 1885 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 24:
#line 163 "hyp_y.y" /* yacc.c:1646  */
    { h.detailed = pcb_false; }
#line 1891 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 25:
#line 164 "hyp_y.y" /* yacc.c:1646  */
    { h.detailed = pcb_true; }
#line 1897 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 26:
#line 169 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_units(&h)) YYERROR; }
#line 1903 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 27:
#line 172 "hyp_y.y" /* yacc.c:1646  */
    { h.unit_system_english = pcb_true; }
#line 1909 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 28:
#line 173 "hyp_y.y" /* yacc.c:1646  */
    { h.unit_system_english = pcb_false; }
#line 1915 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 29:
#line 176 "hyp_y.y" /* yacc.c:1646  */
    { h.metal_thickness_weight = pcb_true; }
#line 1921 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 30:
#line 177 "hyp_y.y" /* yacc.c:1646  */
    { h.metal_thickness_weight = pcb_false; }
#line 1927 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 31:
#line 181 "hyp_y.y" /* yacc.c:1646  */
    { h.default_plane_separation = yylval.floatval; }
#line 1933 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 32:
#line 181 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_plane_sep(&h)) YYERROR; }
#line 1939 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 38:
#line 195 "hyp_y.y" /* yacc.c:1646  */
    { hyyerror("warning: missing ')'"); }
#line 1945 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 43:
#line 205 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_perimeter_segment(&h)) YYERROR; }
#line 1951 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 44:
#line 208 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_perimeter_arc(&h)) YYERROR; }
#line 1957 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 45:
#line 211 "hyp_y.y" /* yacc.c:1646  */
    { h.name = yylval.strval; }
#line 1963 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 46:
#line 211 "hyp_y.y" /* yacc.c:1646  */
    { h.value = yylval.strval; }
#line 1969 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 47:
#line 211 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_board_attribute(&h)) YYERROR; }
#line 1975 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 56:
#line 230 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_options(&h)) YYERROR; }
#line 1981 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 57:
#line 233 "hyp_y.y" /* yacc.c:1646  */
    { h.use_die_for_metal = yylval.boolval; }
#line 1987 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 60:
#line 238 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 1993 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 61:
#line 238 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_signal(&h)) YYERROR; }
#line 1999 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 66:
#line 247 "hyp_y.y" /* yacc.c:1646  */
    { h.bulk_resistivity = yylval.floatval; h.bulk_resistivity_set = pcb_true; }
#line 2005 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 74:
#line 257 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2011 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 75:
#line 257 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_dielectric(&h)) YYERROR; }
#line 2017 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 79:
#line 265 "hyp_y.y" /* yacc.c:1646  */
    { h.epsilon_r = yylval.floatval; h.epsilon_r_set = pcb_true; }
#line 2023 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 86:
#line 275 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2029 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 87:
#line 275 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_plane(&h)) YYERROR; }
#line 2035 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 91:
#line 283 "hyp_y.y" /* yacc.c:1646  */
    { h.bulk_resistivity = yylval.floatval; h.bulk_resistivity_set = pcb_true; }
#line 2041 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 99:
#line 293 "hyp_y.y" /* yacc.c:1646  */
    { h.thickness = yylval.floatval; h.thickness_set = pcb_true; }
#line 2047 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 100:
#line 296 "hyp_y.y" /* yacc.c:1646  */
    { h.plating_thickness = yylval.floatval; h.plating_thickness_set = pcb_true; }
#line 2053 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 101:
#line 299 "hyp_y.y" /* yacc.c:1646  */
    { h.bulk_resistivity = yylval.floatval; h.bulk_resistivity_set = pcb_true; }
#line 2059 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 102:
#line 302 "hyp_y.y" /* yacc.c:1646  */
    { h.temperature_coefficient = yylval.floatval; h.temperature_coefficient_set = pcb_true; }
#line 2065 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 103:
#line 305 "hyp_y.y" /* yacc.c:1646  */
    { h.epsilon_r = yylval.floatval; h.epsilon_r_set = pcb_true; }
#line 2071 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 104:
#line 308 "hyp_y.y" /* yacc.c:1646  */
    { h.loss_tangent = yylval.floatval; h.loss_tangent_set = pcb_true; }
#line 2077 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 105:
#line 311 "hyp_y.y" /* yacc.c:1646  */
    { h.layer_name = yylval.strval; h.layer_name_set = pcb_true; }
#line 2083 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 106:
#line 314 "hyp_y.y" /* yacc.c:1646  */
    { h.material_name = yylval.strval; h.material_name_set = pcb_true; }
#line 2089 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 107:
#line 317 "hyp_y.y" /* yacc.c:1646  */
    { h.plane_separation = yylval.floatval; h.plane_separation_set = pcb_true; }
#line 2095 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 108:
#line 320 "hyp_y.y" /* yacc.c:1646  */
    { h.conformal = yylval.boolval; h.conformal_set = pcb_true; }
#line 2101 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 109:
#line 323 "hyp_y.y" /* yacc.c:1646  */
    { h.prepreg = yylval.boolval; h.prepreg_set = pcb_true; }
#line 2107 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 114:
#line 336 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2113 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 115:
#line 336 "hyp_y.y" /* yacc.c:1646  */
    { h.device_type = yylval.strval; }
#line 2119 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 116:
#line 336 "hyp_y.y" /* yacc.c:1646  */
    { h.ref = yylval.strval; }
#line 2125 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 117:
#line 336 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_devices(&h)) YYERROR; }
#line 2131 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 125:
#line 355 "hyp_y.y" /* yacc.c:1646  */
    { h.name = yylval.strval; h.name_set = pcb_true; }
#line 2137 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 128:
#line 363 "hyp_y.y" /* yacc.c:1646  */
    { h.value_float = yylval.floatval; h.value_float_set = pcb_true; }
#line 2143 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 129:
#line 366 "hyp_y.y" /* yacc.c:1646  */
    { h.value_string = yylval.strval; h.value_string_set = pcb_true; }
#line 2149 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 130:
#line 369 "hyp_y.y" /* yacc.c:1646  */
    { h.package = yylval.strval; h.package_set = pcb_true; }
#line 2155 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 134:
#line 381 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_supplies(&h)) YYERROR; }
#line 2161 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 136:
#line 385 "hyp_y.y" /* yacc.c:1646  */
    { h.voltage_specified = yylval.boolval; }
#line 2167 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 137:
#line 388 "hyp_y.y" /* yacc.c:1646  */
    { h.conversion = yylval.boolval; }
#line 2173 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 138:
#line 393 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2179 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 139:
#line 393 "hyp_y.y" /* yacc.c:1646  */
    { h.padstack_name = yylval.strval; h.padstack_name_set = pcb_true; }
#line 2185 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 140:
#line 393 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_padstack_end(&h)) YYERROR; }
#line 2191 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 141:
#line 396 "hyp_y.y" /* yacc.c:1646  */
    { h.drill_size = yylval.floatval; h.drill_size_set = pcb_true; }
#line 2197 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 147:
#line 405 "hyp_y.y" /* yacc.c:1646  */
    { h.layer_name = yylval.strval; h.layer_name_set = pcb_true; }
#line 2203 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 148:
#line 405 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_padstack_element(&h)) YYERROR; new_record(); }
#line 2209 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 150:
#line 409 "hyp_y.y" /* yacc.c:1646  */
    { h.pad_shape = yylval.floatval; }
#line 2215 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 152:
#line 410 "hyp_y.y" /* yacc.c:1646  */
    { h.pad_shape = -1; }
#line 2221 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 153:
#line 414 "hyp_y.y" /* yacc.c:1646  */
    { h.pad_sx = yylval.floatval; }
#line 2227 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 154:
#line 414 "hyp_y.y" /* yacc.c:1646  */
    { h.pad_sy = yylval.floatval; }
#line 2233 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 155:
#line 414 "hyp_y.y" /* yacc.c:1646  */
    { h.pad_angle = yylval.floatval; }
#line 2239 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 157:
#line 418 "hyp_y.y" /* yacc.c:1646  */
    { h.pad_type = PAD_TYPE_METAL; h.pad_type_set = pcb_true; }
#line 2245 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 158:
#line 419 "hyp_y.y" /* yacc.c:1646  */
    { h.pad_type = PAD_TYPE_ANTIPAD; h.pad_type_set = pcb_true; }
#line 2251 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 159:
#line 420 "hyp_y.y" /* yacc.c:1646  */
    { h.thermal_clear_shape = yylval.floatval; }
#line 2257 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 160:
#line 421 "hyp_y.y" /* yacc.c:1646  */
    { h.thermal_clear_sx = yylval.floatval; }
#line 2263 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 161:
#line 422 "hyp_y.y" /* yacc.c:1646  */
    { h.thermal_clear_sy = yylval.floatval; }
#line 2269 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 162:
#line 423 "hyp_y.y" /* yacc.c:1646  */
    { h.thermal_clear_angle = yylval.floatval; }
#line 2275 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 163:
#line 424 "hyp_y.y" /* yacc.c:1646  */
    { h.pad_type = PAD_TYPE_THERMAL_RELIEF; h.pad_type_set = pcb_true; }
#line 2281 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 164:
#line 430 "hyp_y.y" /* yacc.c:1646  */
    { h.net_name = yylval.strval; if (exec_net(&h)) YYERROR; }
#line 2287 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 166:
#line 433 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_net_plane_separation(&h)) YYERROR; }
#line 2293 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 170:
#line 438 "hyp_y.y" /* yacc.c:1646  */
    { hyyerror("warning: empty net"); }
#line 2299 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 186:
#line 461 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2305 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 187:
#line 461 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_seg(&h)) YYERROR; }
#line 2311 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 188:
#line 464 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2317 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 189:
#line 464 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_arc(&h)) YYERROR; }
#line 2323 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 194:
#line 477 "hyp_y.y" /* yacc.c:1646  */
    { h.width = yylval.floatval; h.width_set = pcb_true; }
#line 2329 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 195:
#line 480 "hyp_y.y" /* yacc.c:1646  */
    { h.left_plane_separation = yylval.floatval; h.left_plane_separation_set = pcb_true; }
#line 2335 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 196:
#line 483 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2341 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 197:
#line 483 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_via(&h)) YYERROR; }
#line 2347 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 201:
#line 494 "hyp_y.y" /* yacc.c:1646  */
    { h.drill_size = yylval.floatval; h.drill_size_set = pcb_true; }
#line 2353 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 204:
#line 497 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad_shape = yylval.strval; h.via_pad_shape_set = pcb_true; }
#line 2359 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 205:
#line 498 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad_sx = yylval.floatval; h.via_pad_sx_set = pcb_true; }
#line 2365 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 206:
#line 499 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad_sy = yylval.floatval; h.via_pad_sy_set = pcb_true; }
#line 2371 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 207:
#line 500 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad_angle = yylval.floatval; h.via_pad_angle_set = pcb_true; }
#line 2377 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 208:
#line 501 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad1_shape = yylval.strval; h.via_pad1_shape_set = pcb_true; }
#line 2383 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 209:
#line 502 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad1_sx = yylval.floatval; h.via_pad1_sx_set = pcb_true; }
#line 2389 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 210:
#line 503 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad1_sy = yylval.floatval; h.via_pad1_sy_set = pcb_true; }
#line 2395 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 211:
#line 504 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad1_angle = yylval.floatval; h.via_pad1_angle_set = pcb_true; }
#line 2401 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 212:
#line 505 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad2_shape = yylval.strval; h.via_pad2_shape_set = pcb_true; }
#line 2407 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 213:
#line 506 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad2_sx = yylval.floatval; h.via_pad2_sx_set = pcb_true; }
#line 2413 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 214:
#line 507 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad2_sy = yylval.floatval; h.via_pad2_sy_set = pcb_true; }
#line 2419 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 215:
#line 508 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad2_angle  = yylval.floatval; h.via_pad2_angle_set = pcb_true; }
#line 2425 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 216:
#line 512 "hyp_y.y" /* yacc.c:1646  */
    { h.padstack_name = yylval.strval; h.padstack_name_set = pcb_true; }
#line 2431 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 217:
#line 515 "hyp_y.y" /* yacc.c:1646  */
    { h.layer1_name = yylval.strval; h.layer1_name_set = pcb_true; }
#line 2437 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 218:
#line 518 "hyp_y.y" /* yacc.c:1646  */
    { h.layer2_name = yylval.strval; h.layer2_name_set = pcb_true; }
#line 2443 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 219:
#line 521 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2449 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 220:
#line 521 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_pin(&h)) YYERROR; }
#line 2455 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 225:
#line 534 "hyp_y.y" /* yacc.c:1646  */
    { h.pin_reference = yylval.strval; h.pin_reference_set = pcb_true; }
#line 2461 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 226:
#line 537 "hyp_y.y" /* yacc.c:1646  */
    { h.pin_function = PIN_SIM_OUT; h.pin_function_set = pcb_true; }
#line 2467 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 227:
#line 538 "hyp_y.y" /* yacc.c:1646  */
    { h.pin_function = PIN_SIM_IN; h.pin_function_set = pcb_true; }
#line 2473 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 228:
#line 539 "hyp_y.y" /* yacc.c:1646  */
    { h.pin_function = PIN_SIM_BOTH; h.pin_function_set = pcb_true; }
#line 2479 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 229:
#line 543 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2485 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 230:
#line 543 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_pad(&h)) YYERROR; }
#line 2491 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 234:
#line 552 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad_shape = yylval.strval; h.via_pad_shape_set = pcb_true; }
#line 2497 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 235:
#line 553 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad_sx = yylval.floatval; h.via_pad_sx_set = pcb_true; }
#line 2503 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 236:
#line 554 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad_sy = yylval.floatval; h.via_pad_sy_set = pcb_true; }
#line 2509 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 237:
#line 555 "hyp_y.y" /* yacc.c:1646  */
    { h.via_pad_angle = yylval.floatval; h.via_pad_angle_set = pcb_true; }
#line 2515 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 238:
#line 559 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2521 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 239:
#line 559 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_useg(&h)) YYERROR; }
#line 2527 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 242:
#line 567 "hyp_y.y" /* yacc.c:1646  */
    { h.zlayer_name = yylval.strval; h.zlayer_name_set = pcb_true; }
#line 2533 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 243:
#line 568 "hyp_y.y" /* yacc.c:1646  */
    { h.width = yylval.floatval; }
#line 2539 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 244:
#line 569 "hyp_y.y" /* yacc.c:1646  */
    { h.length = yylval.floatval; }
#line 2545 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 246:
#line 574 "hyp_y.y" /* yacc.c:1646  */
    { h.impedance = yylval.floatval; h.impedance_set = pcb_true; }
#line 2551 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 247:
#line 575 "hyp_y.y" /* yacc.c:1646  */
    { h.delay = yylval.floatval; }
#line 2557 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 249:
#line 579 "hyp_y.y" /* yacc.c:1646  */
    { h.resistance = yylval.floatval; h.resistance_set = pcb_true;}
#line 2563 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 252:
#line 585 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2569 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 253:
#line 585 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_polygon_begin(&h)) YYERROR; }
#line 2575 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 254:
#line 586 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_polygon_end(&h)) YYERROR; }
#line 2581 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 261:
#line 601 "hyp_y.y" /* yacc.c:1646  */
    { h.id = yylval.intval; h.id_set = pcb_true; }
#line 2587 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 262:
#line 605 "hyp_y.y" /* yacc.c:1646  */
    { h.polygon_type = POLYGON_TYPE_POUR; h.polygon_type_set = pcb_true; }
#line 2593 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 263:
#line 606 "hyp_y.y" /* yacc.c:1646  */
    { h.polygon_type = POLYGON_TYPE_PLANE; h.polygon_type_set = pcb_true; }
#line 2599 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 264:
#line 607 "hyp_y.y" /* yacc.c:1646  */
    { h.polygon_type = POLYGON_TYPE_COPPER; h.polygon_type_set = pcb_true; }
#line 2605 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 265:
#line 611 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2611 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 266:
#line 611 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_polyvoid_begin(&h)) YYERROR; }
#line 2617 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 267:
#line 612 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_polyvoid_end(&h)) YYERROR; }
#line 2623 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 268:
#line 615 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2629 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 269:
#line 615 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_polyline_begin(&h)) YYERROR; }
#line 2635 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 270:
#line 616 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_polyline_end(&h)) YYERROR; }
#line 2641 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 276:
#line 630 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2647 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 277:
#line 630 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_line(&h)) YYERROR; }
#line 2653 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 278:
#line 633 "hyp_y.y" /* yacc.c:1646  */
    { new_record(); }
#line 2659 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 279:
#line 633 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_curve(&h)) YYERROR; }
#line 2665 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 280:
#line 636 "hyp_y.y" /* yacc.c:1646  */
    { h.name = yylval.strval; }
#line 2671 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 281:
#line 636 "hyp_y.y" /* yacc.c:1646  */
    { h.value = yylval.strval; }
#line 2677 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 282:
#line 636 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_net_attribute(&h)) YYERROR; }
#line 2683 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 283:
#line 641 "hyp_y.y" /* yacc.c:1646  */
    { h.net_class_name = yylval.strval; if (exec_net_class(&h)) YYERROR; }
#line 2689 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 292:
#line 660 "hyp_y.y" /* yacc.c:1646  */
    { h.net_name = yylval.strval; }
#line 2695 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 293:
#line 660 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_net_class_element(&h)) YYERROR; }
#line 2701 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 294:
#line 663 "hyp_y.y" /* yacc.c:1646  */
    { h.name = yylval.strval; }
#line 2707 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 295:
#line 663 "hyp_y.y" /* yacc.c:1646  */
    { h.value = yylval.strval; }
#line 2713 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 296:
#line 663 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_net_class_attribute(&h)) YYERROR; }
#line 2719 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 297:
#line 668 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_end(&h)) YYERROR; }
#line 2725 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 298:
#line 673 "hyp_y.y" /* yacc.c:1646  */
    { h.key = yylval.strval; }
#line 2731 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 299:
#line 673 "hyp_y.y" /* yacc.c:1646  */
    { if (exec_key(&h)) YYERROR; }
#line 2737 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 300:
#line 678 "hyp_y.y" /* yacc.c:1646  */
    { h.x = yylval.floatval; }
#line 2743 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 301:
#line 678 "hyp_y.y" /* yacc.c:1646  */
    { h.y = yylval.floatval; }
#line 2749 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 302:
#line 681 "hyp_y.y" /* yacc.c:1646  */
    { h.x1 = yylval.floatval; }
#line 2755 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 303:
#line 681 "hyp_y.y" /* yacc.c:1646  */
    { h.y1 = yylval.floatval; }
#line 2761 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 304:
#line 684 "hyp_y.y" /* yacc.c:1646  */
    { h.x2 = yylval.floatval; }
#line 2767 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 305:
#line 684 "hyp_y.y" /* yacc.c:1646  */
    { h.y2 = yylval.floatval; }
#line 2773 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 307:
#line 690 "hyp_y.y" /* yacc.c:1646  */
    { h.xc = yylval.floatval; }
#line 2779 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 308:
#line 690 "hyp_y.y" /* yacc.c:1646  */
    { h.yc = yylval.floatval; }
#line 2785 "hyp_y.c" /* yacc.c:1646  */
    break;

  case 309:
#line 690 "hyp_y.y" /* yacc.c:1646  */
    { h.r = yylval.floatval; }
#line 2791 "hyp_y.c" /* yacc.c:1646  */
    break;


#line 2795 "hyp_y.c" /* yacc.c:1646  */
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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

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

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
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
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 692 "hyp_y.y" /* yacc.c:1906  */


/*
 * Supporting C routines
 */

void hyyerror(const char *msg)
{
  /* log pcb-rnd message */
  hyp_error(msg);
}

void hyyprint(FILE *file, int type, YYSTYPE value)
{
  if (type == H_STRING)
    fprintf (file, "%s", value.strval);
   else if (type == H_FLOAT)
    fprintf (file, "%g", value.floatval);
   else if (type == H_BOOL)
    fprintf (file, "%i", value.boolval);
  return;
}

/*
 * reset parse_param struct at beginning of record
 */

void new_record()
{
  h.vers = 0;
  h.detailed = pcb_false;
  h.unit_system_english = pcb_false;
  h.metal_thickness_weight = pcb_false;
  h.default_plane_separation = 0;
  h.use_die_for_metal = pcb_false;
  h.bulk_resistivity = 0;
  h.conformal = pcb_false;
  h.epsilon_r = 0;
  h.layer_name = NULL;
  h.loss_tangent = 0;
  h.material_name = NULL;
  h.plane_separation = 0;
  h.plating_thickness = 0;
  h.prepreg = pcb_false;
  h.temperature_coefficient = 0;
  h.thickness = 0;
  h.bulk_resistivity_set = pcb_false;
  h.conformal_set = pcb_false;
  h.epsilon_r_set = pcb_false;
  h.layer_name_set = pcb_false;
  h.loss_tangent_set = pcb_false;
  h.material_name_set = pcb_false;
  h.plane_separation_set = pcb_false;
  h.plating_thickness_set = pcb_false;
  h.prepreg_set = pcb_false;
  h.temperature_coefficient_set = pcb_false;
  h.thickness_set = pcb_false;
  h.device_type = NULL;
  h.ref = NULL;
  h.value_float = 0;
  h.value_string = NULL;
  h.package = NULL;
  h.name_set = pcb_false;
  h.value_float_set = pcb_false;
  h.value_string_set = pcb_false;
  h.package_set = pcb_false;
  h.voltage_specified = pcb_false;
  h.conversion = pcb_false;
  h.padstack_name = NULL;
  h.drill_size = 0;
  h.pad_shape = 0;
  h.pad_sx = 0;
  h.pad_sy = 0;
  h.pad_angle = 0;
  h.thermal_clear_shape = 0;
  h.thermal_clear_sx = 0;
  h.thermal_clear_sy = 0;
  h.thermal_clear_angle = 0;
  h.pad_type = PAD_TYPE_METAL;
  h.padstack_name_set = pcb_false;
  h.drill_size_set = pcb_false;
  h.pad_type_set = pcb_false;
  h.width = 0;
  h.left_plane_separation = 0;
  h.width_set = pcb_false;
  h.left_plane_separation_set = pcb_false;
  h.layer1_name = NULL;
  h.layer1_name_set = pcb_false;
  h.layer2_name = NULL;
  h.layer2_name_set = pcb_false;
  h.via_pad_shape = NULL;
  h.via_pad_shape_set = pcb_false;
  h.via_pad_sx = 0;
  h.via_pad_sx_set = pcb_false;
  h.via_pad_sy = 0;
  h.via_pad_sy_set = pcb_false;
  h.via_pad_angle = 0;
  h.via_pad_angle_set = pcb_false;
  h.via_pad1_shape = NULL;
  h.via_pad1_shape_set = pcb_false;
  h.via_pad1_sx = 0;
  h.via_pad1_sx_set = pcb_false;
  h.via_pad1_sy = 0;
  h.via_pad1_sy_set = pcb_false;
  h.via_pad1_angle = 0;
  h.via_pad1_angle_set = pcb_false;
  h.via_pad2_shape = NULL;
  h.via_pad2_shape_set = pcb_false;
  h.via_pad2_sx = 0;
  h.via_pad2_sx_set = pcb_false;
  h.via_pad2_sy = 0;
  h.via_pad2_sy_set = pcb_false;
  h.via_pad2_angle = 0;
  h.via_pad2_angle_set = pcb_false;
  h.pin_reference = NULL;
  h.pin_reference_set = pcb_false;
  h.pin_function = PIN_SIM_BOTH;
  h.pin_function_set = pcb_false;
  h.zlayer_name = NULL;
  h.zlayer_name_set = pcb_false;
  h.length = 0;
  h.impedance = 0;
  h.impedance_set = pcb_false;
  h.delay = 0;
  h.resistance = 0;
  h.resistance_set = pcb_false;
  h.id = -1;
  h.id_set = pcb_false;
  h.polygon_type = POLYGON_TYPE_PLANE;
  h.polygon_type_set = pcb_false;
  h.net_class_name = NULL;
  h.net_name = NULL;
  h.key = NULL;
  h.name = NULL;
  h.value = NULL;
  h.x = 0;
  h.y = 0;
  h.x1 = 0;
  h.y1 = 0;
  h.x2 = 0;
  h.y2 = 0;
  h.xc = 0;
  h.yc = 0;
  h.r = 0;

  return;
}

/* not truncated */
