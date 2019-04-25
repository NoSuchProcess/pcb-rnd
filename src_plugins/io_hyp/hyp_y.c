/* A Bison parser, made by GNU Bison 3.3.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.3.2"

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
#line 21 "hyp_y.y" /* yacc.c:352  */

#include "parser.h"

#line 123 "hyp_y.c" /* yacc.c:352  */

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
#line 30 "hyp_y.y" /* yacc.c:352  */

    int boolval;
    int intval;
    double floatval;
    char* strval;

#line 246 "hyp_y.c" /* yacc.c:352  */
};

typedef union HYYSTYPE HYYSTYPE;
# define HYYSTYPE_IS_TRIVIAL 1
# define HYYSTYPE_IS_DECLARED 1
#endif


extern HYYSTYPE hyylval;

int hyyparse (void);

#endif /* !YY_HYY_HYP_Y_H_INCLUDED  */

/* Second part of user prologue.  */
#line 37 "hyp_y.y" /* yacc.c:354  */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "hyp_l.h"

void hyyerror(const char *);

/* HYYPRINT and hyyprint print values of the tokens when debugging is switched on */
void hyyprint(FILE *, int, HYYSTYPE);
#define HYYPRINT(file, type, value) hyyprint (file, type, value)

/* clear parse_param struct at beginning of new record */
static void new_record();

/* struct to pass to calling class */
static parse_param h;


#line 283 "hyp_y.c" /* yacc.c:354  */

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
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
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
#  define YYSIZE_T unsigned
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

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
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
#define YYFINAL  34
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   769

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  110
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  179
/* YYNRULES -- Number of rules.  */
#define YYNRULES  311
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  620

#define YYUNDEFTOK  2
#define YYMAXUTOK   358

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
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
       2,     2,     2,   105,     2,   104,     2,     2,     2,     2,
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
       0,   128,   128,   129,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   151,
     151,   156,   156,   161,   164,   165,   170,   173,   174,   177,
     178,   182,   182,   187,   188,   191,   192,   195,   196,   197,
     201,   202,   203,   206,   209,   212,   212,   212,   217,   220,
     221,   224,   225,   226,   227,   228,   231,   234,   234,   235,
     239,   239,   242,   243,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,   258,   258,   261,   262,   265,   266,
     267,   268,   269,   270,   271,   272,   276,   276,   279,   280,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   294,
     297,   300,   303,   306,   309,   312,   315,   318,   321,   324,
     329,   330,   333,   334,   337,   337,   337,   337,   338,   341,
     342,   346,   347,   351,   352,   356,   359,   360,   364,   367,
     370,   375,   378,   379,   382,   383,   386,   389,   394,   394,
     394,   397,   397,   398,   399,   402,   403,   406,   406,   407,
     410,   410,   411,   415,   415,   415,   418,   419,   420,   421,
     422,   423,   424,   421,   431,   431,   434,   434,   435,   439,
     440,   440,   444,   445,   448,   449,   450,   451,   452,   453,
     454,   455,   456,   457,   458,   459,   463,   463,   466,   466,
     469,   470,   474,   475,   479,   482,   485,   485,   489,   490,
     494,   496,   497,   498,   499,   500,   501,   502,   503,   504,
     505,   506,   507,   508,   509,   510,   514,   517,   520,   523,
     523,   526,   527,   531,   532,   536,   539,   540,   541,   542,
     546,   546,   549,   550,   554,   555,   556,   557,   558,   562,
     562,   565,   566,   570,   571,   572,   570,   577,   578,   577,
     582,   582,   584,   588,   588,   588,   592,   593,   597,   598,
     599,   600,   604,   608,   609,   610,   614,   614,   614,   618,
     618,   618,   622,   623,   627,   628,   629,   630,   634,   634,
     637,   637,   640,   640,   640,   645,   645,   648,   649,   653,
     654,   658,   659,   660,   664,   664,   667,   667,   667,   672,
     677,   677,   682,   682,   685,   685,   688,   688,   691,   694,
     694,   694
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
  "H_POSINT", "H_FLOAT", "H_STRING", "'}'", "'{'", "'='", "'('", "')'",
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
     355,   356,   357,   358,   125,   123,    61,    40,    41,    44
};
# endif

#define YYPACT_NINF -454

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-454)))

#define YYTABLE_NINF -115

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       9,   -75,   295,    15,  -454,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,
    -454,   -65,   -31,   -20,     5,     8,    30,   120,    74,  -454,
      97,   113,    -2,   117,  -454,  -454,    22,   127,   161,   207,
     144,   152,  -454,   179,    12,  -454,   167,   140,  -454,  -454,
    -454,  -454,  -454,  -454,    18,   181,  -454,    47,   188,  -454,
     158,   168,   204,  -454,   225,  -454,  -454,  -454,  -454,   182,
    -454,  -454,   115,  -454,  -454,   286,   242,   242,   208,  -454,
    -454,  -454,  -454,  -454,   222,  -454,    28,  -454,  -454,  -454,
    -454,   223,   230,  -454,  -454,   224,   270,  -454,  -454,   232,
    -454,  -454,  -454,   233,  -454,  -454,  -454,   234,   235,   236,
     237,   248,   251,  -454,  -454,  -454,  -454,   210,   238,  -454,
    -454,   190,   170,  -454,  -454,  -454,   239,   257,  -454,   -41,
     213,   243,  -454,  -454,  -454,   245,   244,   246,  -454,   247,
     249,   250,   252,   253,   254,   255,   256,   258,   122,  -454,
    -454,  -454,  -454,  -454,  -454,  -454,  -454,   263,   259,   260,
     261,   262,   108,  -454,  -454,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,   264,   265,    26,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,   279,   266,   267,   268,    58,
      89,   160,  -454,  -454,  -454,   271,   174,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,   146,
    -454,   218,  -454,  -454,  -454,  -454,  -454,  -454,   272,   274,
     275,   278,   277,   269,   280,   281,   283,   284,  -454,  -454,
    -454,   285,   287,   288,   289,  -454,  -454,   290,   291,  -454,
    -454,   282,  -454,   292,   293,   298,    19,   110,   276,   294,
    -454,   296,  -454,  -454,  -454,   273,  -454,   326,  -454,  -454,
    -454,  -454,  -454,    88,  -454,  -454,  -454,   297,   329,   357,
    -454,  -454,   313,   305,  -454,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,   299,  -454,  -454,  -454,  -454,  -454,
    -454,   300,  -454,   302,   303,   304,   306,  -454,  -454,   294,
    -454,  -454,  -454,   197,   197,   340,  -454,   312,   307,   242,
     312,   242,   242,   312,  -454,  -454,   309,   310,   311,   314,
     316,   317,  -454,  -454,  -454,   318,  -454,  -454,   315,   294,
     319,   320,   321,  -454,  -454,   157,  -454,  -454,  -454,   157,
     312,   322,    60,   301,   334,   337,   334,   352,    79,   327,
     328,   330,   332,   323,   331,    85,  -454,   -62,   294,   335,
     171,   333,  -454,  -454,  -454,  -454,   336,   338,   339,   341,
     342,  -454,    21,  -454,  -454,   363,   343,    -5,   363,   344,
     248,   345,   346,   347,   348,   349,   350,   351,   353,   354,
     355,   356,   358,   359,   360,   361,    -4,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,   366,   367,   364,   373,   324,
    -454,  -454,   -22,   363,  -454,  -454,  -454,  -454,   369,  -454,
    -454,  -454,  -454,  -454,   159,   159,   159,  -454,   370,   371,
     374,   375,  -454,  -454,   376,   -43,   372,   377,  -454,   -28,
    -454,  -454,   365,   -43,   378,   362,   380,   381,   383,   384,
     385,   386,   387,   389,   390,   391,   393,   394,   395,   397,
     398,  -454,  -454,   396,   399,  -454,  -454,  -454,   172,   382,
    -454,  -454,  -454,  -454,   388,  -454,   185,   109,  -454,   176,
    -454,  -454,  -454,   183,   211,   403,  -454,  -454,  -454,  -454,
     400,   402,  -454,    -8,  -454,  -454,   401,  -454,   -21,  -454,
    -454,  -454,  -454,   229,  -454,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,   404,
    -454,   410,  -454,   408,  -454,   392,  -454,    -6,  -454,   405,
    -454,  -454,  -454,  -454,  -454,  -454,   406,   411,   413,  -454,
    -454,  -454,  -454,  -454,  -454,   412,   414,  -454,  -454,  -454,
     416,   415,  -454,   420,   409,   417,  -454,  -454,   242,   312,
     421,  -454,  -454,   422,   423,  -454,   425,  -454,  -454,  -454,
     407,   424,   426,  -454,   427,  -454,  -454,   428,  -454,   419,
     429,  -454,  -454,  -454,   448,   432,  -454,   431,  -454,   433,
     434,  -454,   435,   436,   439,   440,  -454,  -454,  -454,   -45,
     441,   437,   442,  -454,  -454,   443,   445,   449,   450,  -454,
    -454,  -454,   444,   446,   447,   451,  -454,  -454,   452,  -454
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,     0,     0,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,     0,     0,     0,     0,     0,     0,     0,     0,   138,
       0,     0,     0,     0,     1,     2,     0,     0,     0,     0,
       0,     0,    34,     0,     0,    36,     0,     0,    50,    51,
      52,    53,    54,   111,     0,     0,   113,     0,     0,   133,
       0,     0,     0,   299,     0,    20,    21,    25,    24,     0,
      27,    28,     0,    31,    39,     0,     0,     0,    38,    40,
      41,    42,    33,    35,     0,    74,     0,    86,    60,    48,
      49,     0,     0,   110,   112,     0,     0,   131,   132,     0,
     164,   285,   300,     0,    23,    30,    29,     0,     0,     0,
       0,     0,     0,    44,    43,    37,    55,     0,     0,    59,
      56,     0,     0,   118,   115,   135,     0,     0,   139,   170,
       0,     0,    22,    26,    32,     0,     0,     0,   308,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    77,
      78,    80,    81,    84,    85,    82,    83,     0,     0,     0,
       0,     0,     0,    89,    90,    92,    93,    94,    95,    96,
      97,    98,     0,     0,     0,    63,    64,    65,    67,    68,
      69,    70,    71,    72,    73,     0,     0,     0,     0,     0,
       0,     0,   166,   165,   168,     0,     0,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   288,     0,
     286,     0,   290,   292,   291,   301,    45,   304,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    75,    76,
      57,     0,     0,     0,     0,    87,    88,     0,     0,    61,
      62,     0,   125,     0,     0,     0,     0,     0,     0,   144,
     146,     0,   253,   269,   266,     0,   230,     0,   188,   219,
     186,   239,   196,   170,   171,   169,   172,     0,     0,     0,
     287,   289,     0,     0,   306,   309,    79,   108,   103,   105,
     104,   106,   109,    99,     0,   101,    91,   107,   102,    66,
     100,     0,   128,     0,     0,     0,     0,   147,   141,   143,
     140,   145,   185,     0,     0,     0,   184,     0,     0,     0,
       0,     0,     0,     0,   167,   293,     0,     0,     0,     0,
       0,     0,    58,   116,   136,     0,   134,   149,     0,     0,
       0,     0,     0,   258,   259,     0,   257,   261,   260,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   137,     0,   142,     0,
       0,     0,   256,   254,   270,   267,     0,     0,     0,     0,
       0,   234,     0,   233,   282,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   199,   200,   202,
     203,   296,   294,    46,   305,     0,     0,     0,   124,     0,
     120,   122,     0,     0,   126,   127,   150,   152,     0,   262,
     265,   264,   263,   194,     0,     0,     0,   302,     0,     0,
       0,     0,   231,   232,     0,     0,     0,     0,   224,     0,
     220,   222,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   197,   198,     0,     0,    47,   307,   310,     0,     0,
     123,   117,   119,   121,     0,   153,     0,     0,   276,     0,
     273,   274,   275,     0,     0,     0,   238,   235,   236,   237,
       0,     0,   193,     0,   189,   191,     0,   225,     0,   221,
     223,   187,   217,     0,   207,   204,   211,   215,   201,   218,
     216,   205,   206,   208,   209,   210,   212,   213,   214,     0,
     295,     0,   129,     0,   151,     0,   156,     0,   148,     0,
     280,   278,   255,   272,   271,   268,     0,     0,     0,   190,
     192,   228,   227,   226,   229,     0,     0,   240,   241,   242,
       0,     0,   130,     0,     0,     0,   159,   277,     0,     0,
       0,   283,   195,     0,     0,   297,     0,   154,   158,   157,
       0,     0,     0,   303,     0,   247,   243,     0,   311,     0,
       0,   281,   279,   284,     0,     0,   298,     0,   160,     0,
       0,   155,     0,     0,     0,     0,   248,   244,   161,     0,
       0,     0,     0,   252,   249,     0,     0,     0,     0,   162,
     250,   245,     0,     0,     0,     0,   251,   246,     0,   163
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -454,  -454,   438,  -454,  -454,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,  -454,   379,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,   456,  -454,  -454,  -454,  -454,
    -454,  -454,   418,  -454,  -454,  -454,   430,  -454,  -454,  -454,
     453,   -79,  -454,   -87,   -73,   -14,    20,  -116,    24,  -122,
    -454,  -454,  -454,  -454,   488,  -454,  -454,  -454,  -454,    31,
      27,    87,  -454,   454,  -454,  -454,  -454,  -454,   487,  -454,
    -454,  -454,  -454,  -454,  -454,  -454,  -230,  -245,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,  -454,   200,  -454,  -454,   308,  -454,  -454,  -454,
    -454,     3,   -13,  -133,  -454,  -454,  -454,  -454,   154,   128,
     209,   112,  -454,  -454,  -454,   119,  -454,  -454,  -454,  -454,
    -454,   187,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,  -454,  -454,   325,  -117,   368,
    -454,  -454,  -454,  -454,  -454,  -454,  -454,   -99,  -453,  -454,
    -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,
     455,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -454,  -302,
    -454,   457,  -454,   184,  -454,   -74,  -307,  -454,  -454
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,     5,    36,     6,   103,     7,    69,     8,
      72,   107,     9,   108,    10,    44,    45,    78,    79,    80,
      81,   272,   465,    11,    47,    48,    49,   120,   284,    50,
     122,   174,   175,    51,   117,   148,   149,    52,   121,   162,
     163,   150,   177,   165,   166,   151,   152,   333,   154,   171,
     155,   156,    12,    55,    56,    92,   185,   355,   409,   410,
     411,   127,   413,   414,   415,   470,    13,    58,    59,   245,
     295,    14,    60,   189,   248,   329,   249,   250,   328,   418,
     474,   476,   525,   579,   528,   570,   592,   601,   612,    15,
     129,   193,   263,   194,   195,   196,   197,   198,   311,   199,
     309,   494,   495,   334,   496,   200,   313,   396,   397,   398,
     399,   400,   201,   310,   440,   441,   377,   442,   202,   307,
     372,   373,   203,   312,   547,   548,   585,   600,   614,   549,
     584,   599,   604,   613,   204,   303,   424,   335,   336,   337,
     338,   205,   305,   426,   206,   304,   425,   479,   480,   481,
     559,   482,   558,   207,   434,   574,    16,   130,   210,   211,
     212,   213,   464,   214,   463,   577,    17,    18,   131,   342,
     485,   111,   273,   138,   320,   112,   113,   321,   521
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     184,   153,   344,   114,   301,   169,   182,   192,   345,    41,
       1,   348,   554,    41,   381,    34,     1,   299,   491,    91,
     296,   541,   542,   543,   602,   160,   533,   160,   437,    19,
     533,   533,   153,   363,   382,   178,   143,   364,   365,   367,
     416,    37,   164,   176,   383,   384,   169,   417,    95,   179,
     385,   437,   184,   491,   301,   379,   386,   555,   182,   368,
     387,   387,   407,   603,   190,   492,   191,   388,   389,   390,
     391,   392,   393,   394,   395,    38,   158,   172,   367,   143,
     438,   142,   544,   164,   143,    96,    39,   178,   144,   145,
     251,   173,   369,   370,   160,   176,   556,   381,   368,   358,
     492,   179,    63,   438,   461,   147,   161,   167,   180,   118,
     529,    40,    42,   301,     2,    43,    82,   382,   143,    43,
       2,  -114,   297,   252,   253,   254,    65,   383,   384,   432,
     530,   369,   370,   385,   239,   531,   119,    46,   379,   386,
     105,   168,   181,   143,   387,   170,   183,   267,   167,   126,
     388,   389,   390,   391,   392,   393,   394,   395,   158,   159,
     180,   255,   106,   142,   268,   246,   143,   247,    84,   407,
     144,   145,   256,   140,   269,   141,   160,   142,   257,   258,
     143,    57,   168,    67,   144,   145,   170,   147,   161,   146,
      85,   420,   259,   190,   181,   191,    86,    75,   183,   260,
      87,   147,    68,    61,   421,   261,   262,    88,   422,    76,
      77,   375,   298,   378,   330,   143,   235,   246,   362,    62,
     158,   172,   362,    64,    53,   142,   371,    54,   143,    66,
     228,    70,   144,   145,    71,   173,   331,   346,   160,   408,
     158,   159,   332,   341,    89,   142,    73,    46,   143,   147,
     161,   571,   144,   145,   330,   143,   371,   572,   160,   435,
      74,   140,   443,   141,    99,   142,   477,   478,   143,   147,
     161,   100,   144,   145,   292,   522,   331,   146,   265,   190,
     532,   191,   332,   477,   478,    93,   104,   534,    54,   147,
     477,   478,    97,   526,   527,    57,   408,   408,    20,    21,
      22,    23,    24,    25,    26,    27,    28,   101,    29,    30,
      31,    32,    33,   493,   109,   535,   115,   208,   477,   478,
     209,   493,   270,   545,   546,   209,   483,   484,   102,   110,
     116,   123,   125,   124,   126,   128,   137,   132,   133,   134,
     139,   187,   135,   136,   157,   186,   217,   215,   216,   241,
     294,   244,   218,   219,   308,   220,   221,   316,   222,   223,
     224,   225,   226,   230,   227,   231,   232,   233,   234,   242,
     237,   238,   279,   243,   274,   264,   275,   276,   277,   278,
     300,   306,   280,   282,   281,   317,   283,   285,   291,   286,
     287,   288,   289,   290,   292,   318,   319,   330,   341,   293,
     302,   246,   324,   323,   374,   315,   376,   322,   353,   325,
     354,   379,   326,   343,   327,   349,   350,   351,   356,   332,
     352,   143,   386,    83,   357,   359,   360,   361,   366,   405,
     401,   402,   471,   403,   404,   423,   419,   406,   427,   469,
     473,    35,   412,   472,   428,   429,   501,   430,   431,   436,
     444,   446,   447,   448,   449,   450,   451,   452,   490,   453,
     454,   455,   456,   314,   457,   458,   459,   460,   466,   467,
     468,   475,   486,   500,   487,   497,   488,   489,   519,   551,
     539,   502,   504,   498,   505,   506,   507,   508,   523,   509,
     510,   511,   512,   536,   513,   514,   515,   524,   516,   517,
     518,   553,   589,    90,   266,   439,   537,   520,   538,   540,
     550,   552,   560,   557,   561,   562,   580,   568,   563,   565,
     564,   566,   567,   573,   575,   569,   576,   578,   587,   590,
     618,   588,   581,   591,   582,   583,   586,   605,   596,   593,
     594,   597,   598,    94,   595,    98,   606,   609,   607,   608,
     462,   610,   611,   615,   616,   617,   380,   503,   499,   433,
     619,     0,     0,     0,   445,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   229,     0,
       0,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   240,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   236,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   339,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   271,     0,     0,     0,
       0,     0,     0,   340,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   347
};

static const yytype_int16 yycheck[] =
{
     122,   117,   309,    77,   249,   121,   122,   129,   310,     1,
       1,   313,    18,     1,    18,     0,     1,   247,    61,     1,
       1,    42,    43,    44,    69,    68,   479,    68,    56,   104,
     483,   484,   148,   335,    38,   122,    58,   339,   340,    18,
     102,   106,   121,   122,    48,    49,   162,   109,     1,   122,
      54,    56,   174,    61,   299,    59,    60,    63,   174,    38,
      65,    65,    84,   108,   105,   108,   107,    71,    72,    73,
      74,    75,    76,    77,    78,   106,    50,    51,    18,    58,
     108,    55,   103,   162,    58,    38,   106,   174,    62,    63,
       1,    65,    71,    72,    68,   174,   102,    18,    38,   329,
     108,   174,   104,   108,   108,    79,    80,   121,   122,    81,
       1,   106,   104,   358,   105,   107,   104,    38,    58,   107,
     105,   103,   103,    34,    35,    36,   104,    48,    49,   108,
      21,    71,    72,    54,   108,    26,   108,   107,    59,    60,
      25,   121,   122,    58,    65,   121,   122,     1,   162,    64,
      71,    72,    73,    74,    75,    76,    77,    78,    50,    51,
     174,     1,    47,    55,    18,   107,    58,   109,     1,    84,
      62,    63,    12,    51,    28,    53,    68,    55,    18,    19,
      58,   107,   162,    22,    62,    63,   162,    79,    80,    67,
      23,    20,    32,   105,   174,   107,    29,    18,   174,    39,
      33,    79,    41,   106,    33,    45,    46,    40,    37,    30,
      31,   344,   102,   346,    57,    58,   108,   107,   335,   106,
      50,    51,   339,   106,   104,    55,   342,   107,    58,   102,
     108,    24,    62,    63,    27,    65,    79,   311,    68,   355,
      50,    51,    85,    86,   104,    55,   102,   107,    58,    79,
      80,   558,    62,    63,    57,    58,   372,   559,    68,   375,
     108,    51,   378,    53,   106,    55,   107,   108,    58,    79,
      80,   103,    62,    63,   102,   103,    79,    67,   104,   105,
     104,   107,    85,   107,   108,   104,   104,   104,   107,    79,
     107,   108,   104,   108,   109,   107,   412,   413,     3,     4,
       5,     6,     7,     8,     9,    10,    11,   103,    13,    14,
      15,    16,    17,   435,    28,   104,   108,   104,   107,   108,
     107,   443,   104,    94,    95,   107,   425,   426,   103,    87,
     108,   108,   108,   103,    64,   103,    88,   104,   104,   104,
      89,    84,   106,   106,   106,   106,   102,   104,   103,    70,
      52,    83,   106,   106,    28,   106,   106,    28,   106,   106,
     106,   106,   106,   100,   106,   106,   106,   106,   106,   103,
     106,   106,   103,   106,   102,   104,   102,   102,   100,   102,
     104,   108,   102,   100,   103,    28,   102,   102,   106,   102,
     102,   102,   102,   102,   102,    82,    91,    57,    86,   106,
     104,   107,   100,   103,   103,   108,    69,   108,    92,   106,
      93,    59,   108,   106,   108,   106,   106,   106,   100,    85,
     106,    58,    60,    44,   109,   106,   106,   106,   106,   106,
     103,   103,   108,   103,   102,   102,   101,   106,   102,    66,
     413,     3,   355,   412,   106,   106,   443,   106,   106,   106,
     106,   106,   106,   106,   106,   106,   106,   106,    82,   106,
     106,   106,   106,   263,   106,   106,   106,   106,   102,   102,
     106,   102,   102,   108,   103,   103,   102,   102,    82,    69,
     493,   103,   102,   106,   103,   102,   102,   102,   106,   103,
     103,   102,   102,    90,   103,   102,   102,   109,   103,   102,
     102,   109,    54,    47,   196,   377,   106,   108,   106,   108,
     106,   103,   106,   108,   103,   102,   109,   108,   106,   103,
     106,   106,   102,   102,   102,   108,   103,   102,   109,    97,
      79,   102,   108,   102,   108,   108,   108,    96,   102,   106,
     106,   102,   102,    55,   109,    58,   109,   102,   106,   106,
     396,   102,   102,   109,   108,   108,   347,   445,   439,   372,
     108,    -1,    -1,    -1,   380,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   148,    -1,
      -1,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   174,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   162,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   304,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   211,    -1,    -1,    -1,
      -1,    -1,    -1,   305,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   312
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     1,   105,   111,   112,   113,   115,   117,   119,   122,
     124,   133,   162,   176,   181,   199,   266,   276,   277,   104,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    13,
      14,    15,    16,    17,     0,   112,   114,   106,   106,   106,
     106,     1,   104,   107,   125,   126,   107,   134,   135,   136,
     139,   143,   147,   104,   107,   163,   164,   107,   177,   178,
     182,   106,   106,   104,   106,   104,   102,    22,    41,   118,
      24,    27,   120,   102,   108,    18,    30,    31,   127,   128,
     129,   130,   104,   126,     1,    23,    29,    33,    40,   104,
     135,     1,   165,   104,   164,     1,    38,   104,   178,   106,
     103,   103,   103,   116,   104,    25,    47,   121,   123,    28,
      87,   281,   285,   286,   285,   108,   108,   144,    81,   108,
     137,   148,   140,   108,   103,   108,    64,   171,   103,   200,
     267,   278,   104,   104,   104,   106,   106,    88,   283,    89,
      51,    53,    55,    58,    62,    63,    67,    79,   145,   146,
     151,   155,   156,   157,   158,   160,   161,   106,    50,    51,
      68,    80,   149,   150,   151,   153,   154,   155,   156,   157,
     158,   159,    51,    65,   141,   142,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   166,   106,    84,   173,   183,
     105,   107,   159,   201,   203,   204,   205,   206,   207,   209,
     215,   222,   228,   232,   244,   251,   254,   263,   104,   107,
     268,   269,   270,   271,   273,   104,   103,   102,   106,   106,
     106,   106,   106,   106,   106,   106,   106,   106,   108,   146,
     100,   106,   106,   106,   106,   108,   150,   106,   106,   108,
     142,    70,   103,   106,    83,   179,   107,   109,   184,   186,
     187,     1,    34,    35,    36,     1,    12,    18,    19,    32,
      39,    45,    46,   202,   104,   104,   206,     1,    18,    28,
     104,   270,   131,   282,   102,   102,   102,   100,   102,   103,
     102,   103,   100,   102,   138,   102,   102,   102,   102,   102,
     102,   106,   102,   106,    52,   180,     1,   103,   102,   186,
     104,   187,   104,   245,   255,   252,   108,   229,    28,   210,
     223,   208,   233,   216,   203,   108,    28,    28,    82,    91,
     284,   287,   108,   103,   100,   106,   108,   108,   188,   185,
      57,    79,    85,   157,   213,   247,   248,   249,   250,   247,
     249,    86,   279,   106,   286,   279,   285,   281,   279,   106,
     106,   106,   106,    92,    93,   167,   100,   109,   186,   106,
     106,   106,   248,   279,   279,   279,   106,    18,    38,    71,
      72,   157,   230,   231,   103,   213,    69,   226,   213,    59,
     220,    18,    38,    48,    49,    54,    60,    65,    71,    72,
      73,    74,    75,    76,    77,    78,   217,   218,   219,   220,
     221,   103,   103,   103,   102,   106,   106,    84,   157,   168,
     169,   170,   171,   172,   173,   174,   102,   109,   189,   101,
      20,    33,    37,   102,   246,   256,   253,   102,   106,   106,
     106,   106,   108,   231,   264,   157,   106,    56,   108,   219,
     224,   225,   227,   157,   106,   283,   106,   106,   106,   106,
     106,   106,   106,   106,   106,   106,   106,   106,   106,   106,
     106,   108,   218,   274,   272,   132,   102,   102,   106,    66,
     175,   108,   169,   170,   190,   102,   191,   107,   108,   257,
     258,   259,   261,   257,   257,   280,   102,   103,   102,   102,
      82,    61,   108,   159,   211,   212,   214,   103,   106,   225,
     108,   211,   103,   221,   102,   103,   102,   102,   102,   103,
     103,   102,   102,   103,   102,   102,   103,   102,   102,    82,
     108,   288,   103,   106,   109,   192,   108,   109,   194,     1,
      21,    26,   104,   258,   104,   104,    90,   106,   106,   212,
     108,    42,    43,    44,   103,    94,    95,   234,   235,   239,
     106,    69,   103,   109,    18,    63,   102,   108,   262,   260,
     106,   103,   102,   106,   106,   103,   106,   102,   108,   108,
     195,   286,   279,   102,   265,   102,   103,   275,   102,   193,
     109,   108,   108,   108,   240,   236,   108,   109,   102,    54,
      97,   102,   196,   106,   106,   109,   102,   102,   102,   241,
     237,   197,    69,   108,   242,    96,   109,   106,   106,   102,
     102,   102,   198,   243,   238,   109,   108,   108,    79,   108
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   110,   111,   111,   112,   112,   112,   112,   112,   112,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   114,
     113,   116,   115,   117,   118,   118,   119,   120,   120,   121,
     121,   123,   122,   124,   124,   125,   125,   126,   126,   126,
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
     222,   224,   224,   225,   225,   226,   227,   227,   227,   227,
     229,   228,   230,   230,   231,   231,   231,   231,   231,   233,
     232,   234,   234,   236,   237,   238,   235,   240,   241,   239,
     243,   242,   242,   245,   246,   244,   247,   247,   248,   248,
     248,   248,   249,   250,   250,   250,   252,   253,   251,   255,
     256,   254,   257,   257,   258,   258,   258,   258,   260,   259,
     262,   261,   264,   265,   263,   267,   266,   268,   268,   269,
     269,   270,   270,   270,   272,   271,   274,   275,   273,   276,
     278,   277,   280,   279,   282,   281,   284,   283,   285,   287,
     288,   286
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     0,
       4,     0,     6,     5,     1,     1,     6,     1,     1,     1,
       1,     0,     6,     4,     3,     2,     1,     3,     2,     2,
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
       6,     2,     1,     2,     1,     3,     3,     3,     3,     3,
       0,     6,     2,     1,     1,     3,     3,     3,     3,     0,
       8,     1,     1,     0,     0,     0,    13,     0,     0,     9,
       0,     5,     1,     0,     0,     8,     2,     1,     1,     1,
       1,     1,     3,     3,     3,     3,     0,     0,     8,     0,
       0,     8,     2,     1,     1,     1,     1,     3,     0,     5,
       0,     5,     0,     0,    11,     0,     6,     2,     1,     2,
       1,     1,     1,     3,     0,     7,     0,     0,    11,     3,
       0,     6,     0,     7,     0,     7,     0,     7,     2,     0,
       0,    12
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


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


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
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
  unsigned long yylno = yyrline[yyrule];
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
                       &yyvsp[(yyi + 1) - (yynrhs)]
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
            else
              goto append;

          append:
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

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
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
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
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
    default: /* Avoid compiler warnings. */
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
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
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
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
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
# else /* defined YYSTACK_RELOCATE */
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
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

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
        case 19:
#line 151 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_board_file(&h)) YYERROR; }
#line 1878 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 21:
#line 156 "hyp_y.y" /* yacc.c:1652  */
    { h.vers = yylval.floatval; }
#line 1884 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 22:
#line 156 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_version(&h)) YYERROR; }
#line 1890 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 23:
#line 161 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_data_mode(&h)) YYERROR; }
#line 1896 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 24:
#line 164 "hyp_y.y" /* yacc.c:1652  */
    { h.detailed = pcb_false; }
#line 1902 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 25:
#line 165 "hyp_y.y" /* yacc.c:1652  */
    { h.detailed = pcb_true; }
#line 1908 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 26:
#line 170 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_units(&h)) YYERROR; }
#line 1914 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 27:
#line 173 "hyp_y.y" /* yacc.c:1652  */
    { h.unit_system_english = pcb_true; }
#line 1920 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 28:
#line 174 "hyp_y.y" /* yacc.c:1652  */
    { h.unit_system_english = pcb_false; }
#line 1926 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 29:
#line 177 "hyp_y.y" /* yacc.c:1652  */
    { h.metal_thickness_weight = pcb_true; }
#line 1932 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 30:
#line 178 "hyp_y.y" /* yacc.c:1652  */
    { h.metal_thickness_weight = pcb_false; }
#line 1938 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 31:
#line 182 "hyp_y.y" /* yacc.c:1652  */
    { h.default_plane_separation = yylval.floatval; }
#line 1944 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 32:
#line 182 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_plane_sep(&h)) YYERROR; }
#line 1950 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 38:
#line 196 "hyp_y.y" /* yacc.c:1652  */
    { hyyerror("warning: missing ')'"); }
#line 1956 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 43:
#line 206 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_perimeter_segment(&h)) YYERROR; }
#line 1962 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 44:
#line 209 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_perimeter_arc(&h)) YYERROR; }
#line 1968 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 45:
#line 212 "hyp_y.y" /* yacc.c:1652  */
    { h.name = yylval.strval; }
#line 1974 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 46:
#line 212 "hyp_y.y" /* yacc.c:1652  */
    { h.value = yylval.strval; }
#line 1980 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 47:
#line 212 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_board_attribute(&h)) YYERROR; }
#line 1986 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 56:
#line 231 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_options(&h)) YYERROR; }
#line 1992 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 57:
#line 234 "hyp_y.y" /* yacc.c:1652  */
    { h.use_die_for_metal = yylval.boolval; }
#line 1998 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 60:
#line 239 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2004 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 61:
#line 239 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_signal(&h)) YYERROR; }
#line 2010 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 66:
#line 248 "hyp_y.y" /* yacc.c:1652  */
    { h.bulk_resistivity = yylval.floatval; h.bulk_resistivity_set = pcb_true; }
#line 2016 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 74:
#line 258 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2022 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 75:
#line 258 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_dielectric(&h)) YYERROR; }
#line 2028 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 79:
#line 266 "hyp_y.y" /* yacc.c:1652  */
    { h.epsilon_r = yylval.floatval; h.epsilon_r_set = pcb_true; }
#line 2034 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 86:
#line 276 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2040 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 87:
#line 276 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_plane(&h)) YYERROR; }
#line 2046 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 91:
#line 284 "hyp_y.y" /* yacc.c:1652  */
    { h.bulk_resistivity = yylval.floatval; h.bulk_resistivity_set = pcb_true; }
#line 2052 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 99:
#line 294 "hyp_y.y" /* yacc.c:1652  */
    { h.thickness = yylval.floatval; h.thickness_set = pcb_true; }
#line 2058 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 100:
#line 297 "hyp_y.y" /* yacc.c:1652  */
    { h.plating_thickness = yylval.floatval; h.plating_thickness_set = pcb_true; }
#line 2064 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 101:
#line 300 "hyp_y.y" /* yacc.c:1652  */
    { h.bulk_resistivity = yylval.floatval; h.bulk_resistivity_set = pcb_true; }
#line 2070 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 102:
#line 303 "hyp_y.y" /* yacc.c:1652  */
    { h.temperature_coefficient = yylval.floatval; h.temperature_coefficient_set = pcb_true; }
#line 2076 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 103:
#line 306 "hyp_y.y" /* yacc.c:1652  */
    { h.epsilon_r = yylval.floatval; h.epsilon_r_set = pcb_true; }
#line 2082 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 104:
#line 309 "hyp_y.y" /* yacc.c:1652  */
    { h.loss_tangent = yylval.floatval; h.loss_tangent_set = pcb_true; }
#line 2088 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 105:
#line 312 "hyp_y.y" /* yacc.c:1652  */
    { h.layer_name = yylval.strval; h.layer_name_set = pcb_true; }
#line 2094 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 106:
#line 315 "hyp_y.y" /* yacc.c:1652  */
    { h.material_name = yylval.strval; h.material_name_set = pcb_true; }
#line 2100 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 107:
#line 318 "hyp_y.y" /* yacc.c:1652  */
    { h.plane_separation = yylval.floatval; h.plane_separation_set = pcb_true; }
#line 2106 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 108:
#line 321 "hyp_y.y" /* yacc.c:1652  */
    { h.conformal = yylval.boolval; h.conformal_set = pcb_true; }
#line 2112 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 109:
#line 324 "hyp_y.y" /* yacc.c:1652  */
    { h.prepreg = yylval.boolval; h.prepreg_set = pcb_true; }
#line 2118 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 114:
#line 337 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2124 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 115:
#line 337 "hyp_y.y" /* yacc.c:1652  */
    { h.device_type = yylval.strval; }
#line 2130 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 116:
#line 337 "hyp_y.y" /* yacc.c:1652  */
    { h.ref = yylval.strval; }
#line 2136 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 117:
#line 337 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_devices(&h)) YYERROR; }
#line 2142 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 125:
#line 356 "hyp_y.y" /* yacc.c:1652  */
    { h.name = yylval.strval; h.name_set = pcb_true; }
#line 2148 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 128:
#line 364 "hyp_y.y" /* yacc.c:1652  */
    { h.value_float = yylval.floatval; h.value_float_set = pcb_true; }
#line 2154 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 129:
#line 367 "hyp_y.y" /* yacc.c:1652  */
    { h.value_string = yylval.strval; h.value_string_set = pcb_true; }
#line 2160 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 130:
#line 370 "hyp_y.y" /* yacc.c:1652  */
    { h.package = yylval.strval; h.package_set = pcb_true; }
#line 2166 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 134:
#line 382 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_supplies(&h)) YYERROR; }
#line 2172 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 136:
#line 386 "hyp_y.y" /* yacc.c:1652  */
    { h.voltage_specified = yylval.boolval; }
#line 2178 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 137:
#line 389 "hyp_y.y" /* yacc.c:1652  */
    { h.conversion = yylval.boolval; }
#line 2184 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 138:
#line 394 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2190 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 139:
#line 394 "hyp_y.y" /* yacc.c:1652  */
    { h.padstack_name = yylval.strval; h.padstack_name_set = pcb_true; }
#line 2196 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 140:
#line 394 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_pstk_end(&h)) YYERROR; }
#line 2202 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 141:
#line 397 "hyp_y.y" /* yacc.c:1652  */
    { h.drill_size = yylval.floatval; h.drill_size_set = pcb_true; }
#line 2208 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 147:
#line 406 "hyp_y.y" /* yacc.c:1652  */
    { h.layer_name = yylval.strval; h.layer_name_set = pcb_true; }
#line 2214 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 148:
#line 406 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_pstk_element(&h)) YYERROR; new_record(); }
#line 2220 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 150:
#line 410 "hyp_y.y" /* yacc.c:1652  */
    { h.pad_shape = yylval.floatval; }
#line 2226 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 152:
#line 411 "hyp_y.y" /* yacc.c:1652  */
    { h.pad_shape = -1; }
#line 2232 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 153:
#line 415 "hyp_y.y" /* yacc.c:1652  */
    { h.pad_sx = yylval.floatval; }
#line 2238 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 154:
#line 415 "hyp_y.y" /* yacc.c:1652  */
    { h.pad_sy = yylval.floatval; }
#line 2244 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 155:
#line 415 "hyp_y.y" /* yacc.c:1652  */
    { h.pad_angle = yylval.floatval; }
#line 2250 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 157:
#line 419 "hyp_y.y" /* yacc.c:1652  */
    { h.pad_type = PAD_TYPE_METAL; h.pad_type_set = pcb_true; }
#line 2256 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 158:
#line 420 "hyp_y.y" /* yacc.c:1652  */
    { h.pad_type = PAD_TYPE_ANTIPAD; h.pad_type_set = pcb_true; }
#line 2262 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 159:
#line 421 "hyp_y.y" /* yacc.c:1652  */
    { h.thermal_clear_shape = yylval.floatval; }
#line 2268 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 160:
#line 422 "hyp_y.y" /* yacc.c:1652  */
    { h.thermal_clear_sx = yylval.floatval; }
#line 2274 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 161:
#line 423 "hyp_y.y" /* yacc.c:1652  */
    { h.thermal_clear_sy = yylval.floatval; }
#line 2280 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 162:
#line 424 "hyp_y.y" /* yacc.c:1652  */
    { h.thermal_clear_angle = yylval.floatval; }
#line 2286 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 163:
#line 425 "hyp_y.y" /* yacc.c:1652  */
    { h.pad_type = PAD_TYPE_THERMAL_RELIEF; h.pad_type_set = pcb_true; }
#line 2292 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 164:
#line 431 "hyp_y.y" /* yacc.c:1652  */
    { h.net_name = yylval.strval; if (exec_net(&h)) YYERROR; }
#line 2298 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 166:
#line 434 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_net_plane_separation(&h)) YYERROR; }
#line 2304 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 170:
#line 440 "hyp_y.y" /* yacc.c:1652  */
    { hyyerror("warning: empty net"); }
#line 2310 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 186:
#line 463 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2316 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 187:
#line 463 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_seg(&h)) YYERROR; }
#line 2322 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 188:
#line 466 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2328 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 189:
#line 466 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_arc(&h)) YYERROR; }
#line 2334 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 194:
#line 479 "hyp_y.y" /* yacc.c:1652  */
    { h.width = yylval.floatval; h.width_set = pcb_true; }
#line 2340 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 195:
#line 482 "hyp_y.y" /* yacc.c:1652  */
    { h.left_plane_separation = yylval.floatval; h.left_plane_separation_set = pcb_true; }
#line 2346 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 196:
#line 485 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2352 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 197:
#line 485 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_via(&h)) YYERROR; }
#line 2358 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 201:
#line 496 "hyp_y.y" /* yacc.c:1652  */
    { h.drill_size = yylval.floatval; h.drill_size_set = pcb_true; }
#line 2364 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 204:
#line 499 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad_shape = yylval.strval; h.via_pad_shape_set = pcb_true; }
#line 2370 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 205:
#line 500 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad_sx = yylval.floatval; h.via_pad_sx_set = pcb_true; }
#line 2376 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 206:
#line 501 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad_sy = yylval.floatval; h.via_pad_sy_set = pcb_true; }
#line 2382 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 207:
#line 502 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad_angle = yylval.floatval; h.via_pad_angle_set = pcb_true; }
#line 2388 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 208:
#line 503 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad1_shape = yylval.strval; h.via_pad1_shape_set = pcb_true; }
#line 2394 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 209:
#line 504 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad1_sx = yylval.floatval; h.via_pad1_sx_set = pcb_true; }
#line 2400 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 210:
#line 505 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad1_sy = yylval.floatval; h.via_pad1_sy_set = pcb_true; }
#line 2406 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 211:
#line 506 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad1_angle = yylval.floatval; h.via_pad1_angle_set = pcb_true; }
#line 2412 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 212:
#line 507 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad2_shape = yylval.strval; h.via_pad2_shape_set = pcb_true; }
#line 2418 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 213:
#line 508 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad2_sx = yylval.floatval; h.via_pad2_sx_set = pcb_true; }
#line 2424 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 214:
#line 509 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad2_sy = yylval.floatval; h.via_pad2_sy_set = pcb_true; }
#line 2430 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 215:
#line 510 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad2_angle  = yylval.floatval; h.via_pad2_angle_set = pcb_true; }
#line 2436 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 216:
#line 514 "hyp_y.y" /* yacc.c:1652  */
    { h.padstack_name = yylval.strval; h.padstack_name_set = pcb_true; }
#line 2442 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 217:
#line 517 "hyp_y.y" /* yacc.c:1652  */
    { h.layer1_name = yylval.strval; h.layer1_name_set = pcb_true; }
#line 2448 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 218:
#line 520 "hyp_y.y" /* yacc.c:1652  */
    { h.layer2_name = yylval.strval; h.layer2_name_set = pcb_true; }
#line 2454 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 219:
#line 523 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2460 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 220:
#line 523 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_pin(&h)) YYERROR; }
#line 2466 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 225:
#line 536 "hyp_y.y" /* yacc.c:1652  */
    { h.pin_reference = yylval.strval; h.pin_reference_set = pcb_true; }
#line 2472 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 226:
#line 539 "hyp_y.y" /* yacc.c:1652  */
    { h.pin_function = PIN_SIM_OUT; h.pin_function_set = pcb_true; }
#line 2478 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 227:
#line 540 "hyp_y.y" /* yacc.c:1652  */
    { h.pin_function = PIN_SIM_IN; h.pin_function_set = pcb_true; }
#line 2484 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 228:
#line 541 "hyp_y.y" /* yacc.c:1652  */
    { h.pin_function = PIN_SIM_BOTH; h.pin_function_set = pcb_true; }
#line 2490 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 229:
#line 542 "hyp_y.y" /* yacc.c:1652  */
    { h.pin_function = PIN_SIM_BOTH; h.pin_function_set = pcb_true; hyyerror("warning: SIM_BOTH assumed"); }
#line 2496 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 230:
#line 546 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2502 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 231:
#line 546 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_pad(&h)) YYERROR; }
#line 2508 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 235:
#line 555 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad_shape = yylval.strval; h.via_pad_shape_set = pcb_true; }
#line 2514 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 236:
#line 556 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad_sx = yylval.floatval; h.via_pad_sx_set = pcb_true; }
#line 2520 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 237:
#line 557 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad_sy = yylval.floatval; h.via_pad_sy_set = pcb_true; }
#line 2526 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 238:
#line 558 "hyp_y.y" /* yacc.c:1652  */
    { h.via_pad_angle = yylval.floatval; h.via_pad_angle_set = pcb_true; }
#line 2532 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 239:
#line 562 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2538 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 240:
#line 562 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_useg(&h)) YYERROR; }
#line 2544 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 243:
#line 570 "hyp_y.y" /* yacc.c:1652  */
    { h.zlayer_name = yylval.strval; h.zlayer_name_set = pcb_true; }
#line 2550 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 244:
#line 571 "hyp_y.y" /* yacc.c:1652  */
    { h.width = yylval.floatval; }
#line 2556 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 245:
#line 572 "hyp_y.y" /* yacc.c:1652  */
    { h.length = yylval.floatval; }
#line 2562 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 247:
#line 577 "hyp_y.y" /* yacc.c:1652  */
    { h.impedance = yylval.floatval; h.impedance_set = pcb_true; }
#line 2568 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 248:
#line 578 "hyp_y.y" /* yacc.c:1652  */
    { h.delay = yylval.floatval; }
#line 2574 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 250:
#line 582 "hyp_y.y" /* yacc.c:1652  */
    { h.resistance = yylval.floatval; h.resistance_set = pcb_true;}
#line 2580 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 253:
#line 588 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2586 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 254:
#line 588 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_polygon_begin(&h)) YYERROR; }
#line 2592 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 255:
#line 589 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_polygon_end(&h)) YYERROR; }
#line 2598 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 262:
#line 604 "hyp_y.y" /* yacc.c:1652  */
    { h.id = yylval.intval; h.id_set = pcb_true; }
#line 2604 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 263:
#line 608 "hyp_y.y" /* yacc.c:1652  */
    { h.polygon_type = POLYGON_TYPE_POUR; h.polygon_type_set = pcb_true; }
#line 2610 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 264:
#line 609 "hyp_y.y" /* yacc.c:1652  */
    { h.polygon_type = POLYGON_TYPE_PLANE; h.polygon_type_set = pcb_true; }
#line 2616 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 265:
#line 610 "hyp_y.y" /* yacc.c:1652  */
    { h.polygon_type = POLYGON_TYPE_COPPER; h.polygon_type_set = pcb_true; }
#line 2622 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 266:
#line 614 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2628 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 267:
#line 614 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_polyvoid_begin(&h)) YYERROR; }
#line 2634 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 268:
#line 615 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_polyvoid_end(&h)) YYERROR; }
#line 2640 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 269:
#line 618 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2646 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 270:
#line 618 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_polyline_begin(&h)) YYERROR; }
#line 2652 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 271:
#line 619 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_polyline_end(&h)) YYERROR; }
#line 2658 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 276:
#line 629 "hyp_y.y" /* yacc.c:1652  */
    { hyyerror("warning: unexpected ')'"); }
#line 2664 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 278:
#line 634 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2670 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 279:
#line 634 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_line(&h)) YYERROR; }
#line 2676 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 280:
#line 637 "hyp_y.y" /* yacc.c:1652  */
    { new_record(); }
#line 2682 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 281:
#line 637 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_curve(&h)) YYERROR; }
#line 2688 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 282:
#line 640 "hyp_y.y" /* yacc.c:1652  */
    { h.name = yylval.strval; }
#line 2694 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 283:
#line 640 "hyp_y.y" /* yacc.c:1652  */
    { h.value = yylval.strval; }
#line 2700 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 284:
#line 640 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_net_attribute(&h)) YYERROR; }
#line 2706 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 285:
#line 645 "hyp_y.y" /* yacc.c:1652  */
    { h.net_class_name = yylval.strval; if (exec_net_class(&h)) YYERROR; }
#line 2712 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 294:
#line 664 "hyp_y.y" /* yacc.c:1652  */
    { h.net_name = yylval.strval; }
#line 2718 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 295:
#line 664 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_net_class_element(&h)) YYERROR; }
#line 2724 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 296:
#line 667 "hyp_y.y" /* yacc.c:1652  */
    { h.name = yylval.strval; }
#line 2730 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 297:
#line 667 "hyp_y.y" /* yacc.c:1652  */
    { h.value = yylval.strval; }
#line 2736 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 298:
#line 667 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_net_class_attribute(&h)) YYERROR; }
#line 2742 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 299:
#line 672 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_end(&h)) YYERROR; }
#line 2748 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 300:
#line 677 "hyp_y.y" /* yacc.c:1652  */
    { h.key = yylval.strval; }
#line 2754 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 301:
#line 677 "hyp_y.y" /* yacc.c:1652  */
    { if (exec_key(&h)) YYERROR; }
#line 2760 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 302:
#line 682 "hyp_y.y" /* yacc.c:1652  */
    { h.x = yylval.floatval; }
#line 2766 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 303:
#line 682 "hyp_y.y" /* yacc.c:1652  */
    { h.y = yylval.floatval; }
#line 2772 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 304:
#line 685 "hyp_y.y" /* yacc.c:1652  */
    { h.x1 = yylval.floatval; }
#line 2778 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 305:
#line 685 "hyp_y.y" /* yacc.c:1652  */
    { h.y1 = yylval.floatval; }
#line 2784 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 306:
#line 688 "hyp_y.y" /* yacc.c:1652  */
    { h.x2 = yylval.floatval; }
#line 2790 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 307:
#line 688 "hyp_y.y" /* yacc.c:1652  */
    { h.y2 = yylval.floatval; }
#line 2796 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 309:
#line 694 "hyp_y.y" /* yacc.c:1652  */
    { h.xc = yylval.floatval; }
#line 2802 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 310:
#line 694 "hyp_y.y" /* yacc.c:1652  */
    { h.yc = yylval.floatval; }
#line 2808 "hyp_y.c" /* yacc.c:1652  */
    break;

  case 311:
#line 694 "hyp_y.y" /* yacc.c:1652  */
    { h.r = yylval.floatval; }
#line 2814 "hyp_y.c" /* yacc.c:1652  */
    break;


#line 2818 "hyp_y.c" /* yacc.c:1652  */
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

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


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
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
#line 696 "hyp_y.y" /* yacc.c:1918  */


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

static void new_record()
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
