/* A Bison parser, made by GNU Bison 2.7.12-4996.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.7.12-4996"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 11 "parse_y.y"

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* grammar to parse ASCII input of PCB description
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"
#include "create.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "mymem.h"
#include "misc.h"
#include "parse_l.h"
#include "polygon.h"
#include "remove.h"
#include "rtree.h"
#include "strflags.h"
#include "thermal.h"

#ifdef HAVE_LIBDMALLOC
# include <dmalloc.h> /* see http://dmalloc.com */
#endif

RCSID("$Id$");

static	LayerTypePtr	Layer;
static	PolygonTypePtr	Polygon;
static	SymbolTypePtr	Symbol;
static	int		pin_num;
static	LibraryMenuTypePtr	Menu;
static	bool			LayerFlag[MAX_LAYER + 2];

extern	char			*yytext;		/* defined by LEX */
extern	PCBTypePtr		yyPCB;
extern	DataTypePtr		yyData;
extern	ElementTypePtr	yyElement;
extern	FontTypePtr		yyFont;
extern	int				yylineno;		/* linenumber */
extern	char			*yyfilename;	/* in this file */

static char *layer_group_string; 

static AttributeListTypePtr attr_list; 

int yyerror(const char *s);
int yylex();
static int check_file_version (int);

static void do_measure (PLMeasure *m, Coord i, double d, int u);
#define M(r,f,d) do_measure (&(r), f, d, 1)

/* Macros for interpreting what "measure" means - integer value only,
   old units (mil), or new units (cmil).  */
#define IV(m) integer_value (m)
#define OU(m) old_units (m)
#define NU(m) new_units (m)

static int integer_value (PLMeasure m);
static Coord old_units (PLMeasure m);
static Coord new_units (PLMeasure m);

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

#include "parse_y.h"


/* Line 371 of yacc.c  */
#line 165 "parse_y.tab.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "parse_y.tab.h".  */
#ifndef YY_YY_PARSE_Y_TAB_H_INCLUDED
# define YY_YY_PARSE_Y_TAB_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     FLOATING = 258,
     INTEGER = 259,
     CHAR_CONST = 260,
     STRING = 261,
     T_FILEVERSION = 262,
     T_PCB = 263,
     T_LAYER = 264,
     T_VIA = 265,
     T_RAT = 266,
     T_LINE = 267,
     T_ARC = 268,
     T_RECTANGLE = 269,
     T_TEXT = 270,
     T_ELEMENTLINE = 271,
     T_ELEMENT = 272,
     T_PIN = 273,
     T_PAD = 274,
     T_GRID = 275,
     T_FLAGS = 276,
     T_SYMBOL = 277,
     T_SYMBOLLINE = 278,
     T_CURSOR = 279,
     T_ELEMENTARC = 280,
     T_MARK = 281,
     T_GROUPS = 282,
     T_STYLES = 283,
     T_POLYGON = 284,
     T_POLYGON_HOLE = 285,
     T_NETLIST = 286,
     T_NET = 287,
     T_CONN = 288,
     T_AREA = 289,
     T_THERMAL = 290,
     T_DRC = 291,
     T_ATTRIBUTE = 292,
     T_UMIL = 293,
     T_CMIL = 294,
     T_MIL = 295,
     T_IN = 296,
     T_NM = 297,
     T_UM = 298,
     T_MM = 299,
     T_M = 300,
     T_KM = 301
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 111 "parse_y.y"

	int		integer;
	double		number;
	char		*string;
	FlagType	flagtype;
	PLMeasure	measure;


/* Line 387 of yacc.c  */
#line 263 "parse_y.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_PARSE_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 291 "parse_y.tab.c"

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
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
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
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec) /* empty */
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif


/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

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
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

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
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  10
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   584

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  51
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  107
/* YYNRULES -- Number of rules.  */
#define YYNRULES  204
/* YYNRULES -- Number of states.  */
#define YYNSTATES  616

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   301

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      49,    50,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    47,     2,    48,     2,     2,     2,     2,     2,     2,
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
      45,    46
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    12,    27,    28,
      31,    32,    35,    37,    38,    39,    42,    43,    48,    53,
      60,    67,    69,    71,    73,    80,    88,    96,   103,   110,
     111,   112,   117,   118,   123,   124,   126,   128,   130,   137,
     145,   155,   160,   165,   166,   171,   172,   177,   182,   183,
     185,   186,   188,   191,   193,   194,   197,   199,   201,   202,
     205,   207,   209,   211,   213,   215,   217,   229,   241,   252,
     262,   271,   282,   293,   294,   305,   307,   308,   310,   313,
     315,   317,   319,   321,   323,   325,   334,   336,   338,   340,
     341,   344,   346,   357,   368,   378,   391,   404,   416,   425,
     435,   445,   446,   456,   457,   460,   461,   467,   468,   471,
     476,   481,   483,   485,   487,   489,   491,   492,   505,   506,
     522,   523,   540,   541,   560,   561,   580,   582,   585,   587,
     589,   591,   593,   595,   604,   613,   624,   635,   641,   647,
     648,   651,   653,   656,   658,   660,   662,   664,   673,   682,
     693,   704,   705,   708,   721,   734,   745,   755,   764,   778,
     792,   804,   815,   817,   819,   821,   824,   828,   835,   842,
     844,   846,   847,   850,   853,   862,   871,   873,   874,   881,
     883,   884,   886,   889,   890,   900,   902,   903,   905,   908,
     913,   919,   921,   922,   924,   926,   928,   931,   934,   937,
     940,   943,   946,   949,   952
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      52,     0,    -1,    53,    -1,    56,    -1,    59,    -1,     1,
      -1,    -1,    54,    61,    62,    63,    67,    68,    69,    70,
      74,    75,    76,    58,    77,   145,    -1,    -1,    55,   111,
      -1,    -1,    57,    77,    -1,    59,    -1,    -1,    -1,    60,
     138,    -1,    -1,     7,    47,     4,    48,    -1,     8,    49,
       6,    50,    -1,     8,    49,     6,   157,   157,    50,    -1,
       8,    47,     6,   157,   157,    48,    -1,    64,    -1,    65,
      -1,    66,    -1,    20,    49,   157,   157,   157,    50,    -1,
      20,    49,   157,   157,   157,     4,    50,    -1,    20,    47,
     157,   157,   157,     4,    48,    -1,    24,    49,   157,   157,
     156,    50,    -1,    24,    47,   157,   157,   156,    48,    -1,
      -1,    -1,    34,    47,   156,    48,    -1,    -1,    35,    47,
     156,    48,    -1,    -1,    71,    -1,    72,    -1,    73,    -1,
      36,    47,   157,   157,   157,    48,    -1,    36,    47,   157,
     157,   157,   157,    48,    -1,    36,    47,   157,   157,   157,
     157,   157,   157,    48,    -1,    21,    49,     4,    50,    -1,
      21,    49,     6,    50,    -1,    -1,    27,    49,     6,    50,
      -1,    -1,    28,    49,     6,    50,    -1,    28,    47,     6,
      48,    -1,    -1,    78,    -1,    -1,    79,    -1,    78,    79,
      -1,    82,    -1,    -1,    80,   154,    -1,    88,    -1,    89,
      -1,    -1,    81,   111,    -1,     1,    -1,    83,    -1,    84,
      -1,    85,    -1,    86,    -1,    87,    -1,    10,    47,   157,
     157,   157,   157,   157,   157,     6,   137,    48,    -1,    10,
      49,   157,   157,   157,   157,   157,   157,     6,     4,    50,
      -1,    10,    49,   157,   157,   157,   157,   157,     6,     4,
      50,    -1,    10,    49,   157,   157,   157,   157,     6,     4,
      50,    -1,    10,    49,   157,   157,   157,     6,     4,    50,
      -1,    11,    47,   157,   157,     4,   157,   157,     4,   137,
      48,    -1,    11,    49,   157,   157,     4,   157,   157,     4,
       4,    50,    -1,    -1,     9,    49,     4,     6,   155,    50,
      49,    90,    91,    50,    -1,    92,    -1,    -1,    93,    -1,
      92,    93,    -1,    95,    -1,    96,    -1,    97,    -1,    98,
      -1,    99,    -1,   100,    -1,    14,    49,   157,   157,   157,
     157,     4,    50,    -1,   103,    -1,   102,    -1,   101,    -1,
      -1,    94,   154,    -1,   104,    -1,    12,    47,   157,   157,
     157,   157,   157,   157,   137,    48,    -1,    12,    49,   157,
     157,   157,   157,   157,   157,     4,    50,    -1,    12,    49,
     157,   157,   157,   157,   157,   157,    50,    -1,    13,    47,
     157,   157,   157,   157,   157,   157,   156,   156,   137,    48,
      -1,    13,    49,   157,   157,   157,   157,   157,   157,   156,
     156,     4,    50,    -1,    13,    49,   157,   157,   157,   157,
     157,   157,   156,     4,    50,    -1,    15,    49,   157,   157,
     156,     6,     4,    50,    -1,    15,    49,   157,   157,   156,
     156,     6,     4,    50,    -1,    15,    47,   157,   157,   156,
     156,     6,   137,    48,    -1,    -1,    29,    49,   137,    50,
      49,   105,   109,   106,    50,    -1,    -1,   106,   107,    -1,
      -1,    30,    49,   108,   109,    50,    -1,    -1,   110,   109,
      -1,    49,   157,   157,    50,    -1,    47,   157,   157,    48,
      -1,   112,    -1,   114,    -1,   116,    -1,   118,    -1,   120,
      -1,    -1,    17,    49,     6,     6,   157,   157,     4,    50,
      49,   113,   122,    50,    -1,    -1,    17,    49,     4,     6,
       6,   157,   157,   157,   157,     4,    50,    49,   115,   122,
      50,    -1,    -1,    17,    49,     4,     6,     6,     6,   157,
     157,   157,   157,     4,    50,    49,   117,   122,    50,    -1,
      -1,    17,    49,     4,     6,     6,     6,   157,   157,   157,
     157,   156,   156,     4,    50,    49,   119,   125,    50,    -1,
      -1,    17,    47,   137,     6,     6,     6,   157,   157,   157,
     157,   156,   156,   137,    48,    49,   121,   125,    50,    -1,
     123,    -1,   122,   123,    -1,   130,    -1,   131,    -1,   132,
      -1,   135,    -1,   136,    -1,    16,    47,   157,   157,   157,
     157,   157,    48,    -1,    16,    49,   157,   157,   157,   157,
     157,    50,    -1,    25,    47,   157,   157,   157,   157,   156,
     156,   157,    48,    -1,    25,    49,   157,   157,   157,   157,
     156,   156,   157,    50,    -1,    26,    47,   157,   157,    48,
      -1,    26,    49,   157,   157,    50,    -1,    -1,   124,   154,
      -1,   126,    -1,   125,   126,    -1,   129,    -1,   128,    -1,
     134,    -1,   133,    -1,    16,    47,   157,   157,   157,   157,
     157,    48,    -1,    16,    49,   157,   157,   157,   157,   157,
      50,    -1,    25,    47,   157,   157,   157,   157,   156,   156,
     157,    48,    -1,    25,    49,   157,   157,   157,   157,   156,
     156,   157,    50,    -1,    -1,   127,   154,    -1,    18,    47,
     157,   157,   157,   157,   157,   157,     6,     6,   137,    48,
      -1,    18,    49,   157,   157,   157,   157,   157,   157,     6,
       6,     4,    50,    -1,    18,    49,   157,   157,   157,   157,
       6,     6,     4,    50,    -1,    18,    49,   157,   157,   157,
     157,     6,     4,    50,    -1,    18,    49,   157,   157,   157,
       6,     4,    50,    -1,    19,    47,   157,   157,   157,   157,
     157,   157,   157,     6,     6,   137,    48,    -1,    19,    49,
     157,   157,   157,   157,   157,   157,   157,     6,     6,     4,
      50,    -1,    19,    49,   157,   157,   157,   157,   157,     6,
       6,     4,    50,    -1,    19,    49,   157,   157,   157,   157,
     157,     6,     4,    50,    -1,     4,    -1,     6,    -1,   139,
      -1,   138,   139,    -1,   140,   142,    50,    -1,    22,    47,
     141,   157,    48,    49,    -1,    22,    49,   141,   157,    50,
      49,    -1,     4,    -1,     5,    -1,    -1,   142,   143,    -1,
     142,   144,    -1,    23,    49,   157,   157,   157,   157,   157,
      50,    -1,    23,    47,   157,   157,   157,   157,   157,    48,
      -1,   146,    -1,    -1,    31,    49,    50,    49,   147,    50,
      -1,   148,    -1,    -1,   149,    -1,   148,   149,    -1,    -1,
      32,    49,     6,     6,    50,    49,   150,   151,    50,    -1,
     152,    -1,    -1,   153,    -1,   152,   153,    -1,    33,    49,
       6,    50,    -1,    37,    49,     6,     6,    50,    -1,     6,
      -1,    -1,     3,    -1,     4,    -1,   156,    -1,   156,    38,
      -1,   156,    39,    -1,   156,    40,    -1,   156,    41,    -1,
     156,    42,    -1,   156,    43,    -1,   156,    44,    -1,   156,
      45,    -1,   156,    46,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   137,   137,   138,   139,   140,   164,   164,   219,   219,
     230,   230,   249,   250,   255,   255,   295,   297,   327,   333,
     339,   368,   369,   370,   373,   381,   394,   426,   432,   438,
     454,   456,   481,   483,   514,   516,   517,   518,   522,   532,
     543,   570,   574,   578,   606,   610,   654,   662,   670,   674,
     675,   679,   680,   684,   685,   685,   686,   687,   689,   689,
     696,   700,   701,   702,   703,   704,   740,   750,   761,   771,
     781,   817,   822,   854,   853,   877,   878,   882,   883,   887,
     888,   889,   890,   891,   892,   894,   899,   900,   901,   902,
     902,   903,   933,   942,   951,   999,  1008,  1017,  1054,  1064,
    1082,  1132,  1131,  1170,  1172,  1177,  1176,  1183,  1185,  1190,
    1194,  1254,  1255,  1256,  1257,  1258,  1266,  1265,  1284,  1283,
    1302,  1301,  1322,  1320,  1344,  1342,  1423,  1424,  1428,  1429,
    1430,  1431,  1432,  1434,  1439,  1444,  1449,  1454,  1459,  1464,
    1464,  1468,  1469,  1473,  1474,  1475,  1476,  1478,  1484,  1491,
    1496,  1501,  1501,  1542,  1554,  1566,  1577,  1593,  1647,  1661,
    1674,  1685,  1696,  1697,  1701,  1702,  1724,  1726,  1742,  1761,
    1762,  1765,  1767,  1768,  1789,  1796,  1812,  1813,  1817,  1822,
    1823,  1827,  1828,  1851,  1850,  1860,  1861,  1865,  1866,  1885,
    1915,  1923,  1924,  1928,  1929,  1934,  1935,  1936,  1937,  1938,
    1939,  1940,  1941,  1942,  1943
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "FLOATING", "INTEGER", "CHAR_CONST",
  "STRING", "T_FILEVERSION", "T_PCB", "T_LAYER", "T_VIA", "T_RAT",
  "T_LINE", "T_ARC", "T_RECTANGLE", "T_TEXT", "T_ELEMENTLINE", "T_ELEMENT",
  "T_PIN", "T_PAD", "T_GRID", "T_FLAGS", "T_SYMBOL", "T_SYMBOLLINE",
  "T_CURSOR", "T_ELEMENTARC", "T_MARK", "T_GROUPS", "T_STYLES",
  "T_POLYGON", "T_POLYGON_HOLE", "T_NETLIST", "T_NET", "T_CONN", "T_AREA",
  "T_THERMAL", "T_DRC", "T_ATTRIBUTE", "T_UMIL", "T_CMIL", "T_MIL", "T_IN",
  "T_NM", "T_UM", "T_MM", "T_M", "T_KM", "'['", "']'", "'('", "')'",
  "$accept", "parse", "parsepcb", "$@1", "$@2", "parsedata", "$@3",
  "pcbfont", "parsefont", "$@4", "pcbfileversion", "pcbname", "pcbgrid",
  "pcbgridold", "pcbgridnew", "pcbhigrid", "pcbcursor", "polyarea",
  "pcbthermal", "pcbdrc", "pcbdrc1", "pcbdrc2", "pcbdrc3", "pcbflags",
  "pcbgroups", "pcbstyles", "pcbdata", "pcbdefinitions", "pcbdefinition",
  "$@5", "$@6", "via", "via_hi_format", "via_2.0_format", "via_1.7_format",
  "via_newformat", "via_oldformat", "rats", "layer", "$@7", "layerdata",
  "layerdefinitions", "layerdefinition", "$@8", "line_hi_format",
  "line_1.7_format", "line_oldformat", "arc_hi_format", "arc_1.7_format",
  "arc_oldformat", "text_oldformat", "text_newformat", "text_hi_format",
  "polygon_format", "$@9", "polygonholes", "polygonhole", "$@10",
  "polygonpoints", "polygonpoint", "element", "element_oldformat", "$@11",
  "element_1.3.4_format", "$@12", "element_newformat", "$@13",
  "element_1.7_format", "$@14", "element_hi_format", "$@15",
  "elementdefinitions", "elementdefinition", "$@16", "relementdefs",
  "relementdef", "$@17", "pin_hi_format", "pin_1.7_format",
  "pin_1.6.3_format", "pin_newformat", "pin_oldformat", "pad_hi_format",
  "pad_1.7_format", "pad_newformat", "pad", "flags", "symbols", "symbol",
  "symbolhead", "symbolid", "symboldata", "symboldefinition",
  "hiressymbol", "pcbnetlist", "pcbnetdef", "nets", "netdefs", "net",
  "$@18", "connections", "conndefs", "conn", "attribute", "opt_string",
  "number", "measure", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,    91,    93,    40,
      41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    51,    52,    52,    52,    52,    54,    53,    55,    53,
      57,    56,    58,    58,    60,    59,    61,    61,    62,    62,
      62,    63,    63,    63,    64,    65,    66,    67,    67,    67,
      68,    68,    69,    69,    70,    70,    70,    70,    71,    72,
      73,    74,    74,    74,    75,    75,    76,    76,    76,    77,
      77,    78,    78,    79,    80,    79,    79,    79,    81,    79,
      79,    82,    82,    82,    82,    82,    83,    84,    85,    86,
      87,    88,    88,    90,    89,    91,    91,    92,    92,    93,
      93,    93,    93,    93,    93,    93,    93,    93,    93,    94,
      93,    93,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   105,   104,   106,   106,   108,   107,   109,   109,   110,
     110,   111,   111,   111,   111,   111,   113,   112,   115,   114,
     117,   116,   119,   118,   121,   120,   122,   122,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   124,
     123,   125,   125,   126,   126,   126,   126,   126,   126,   126,
     126,   127,   126,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   137,   138,   138,   139,   140,   140,   141,
     141,   142,   142,   142,   143,   144,   145,   145,   146,   147,
     147,   148,   148,   150,   149,   151,   151,   152,   152,   153,
     154,   155,   155,   156,   156,   157,   157,   157,   157,   157,
     157,   157,   157,   157,   157
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     0,    14,     0,     2,
       0,     2,     1,     0,     0,     2,     0,     4,     4,     6,
       6,     1,     1,     1,     6,     7,     7,     6,     6,     0,
       0,     4,     0,     4,     0,     1,     1,     1,     6,     7,
       9,     4,     4,     0,     4,     0,     4,     4,     0,     1,
       0,     1,     2,     1,     0,     2,     1,     1,     0,     2,
       1,     1,     1,     1,     1,     1,    11,    11,    10,     9,
       8,    10,    10,     0,    10,     1,     0,     1,     2,     1,
       1,     1,     1,     1,     1,     8,     1,     1,     1,     0,
       2,     1,    10,    10,     9,    12,    12,    11,     8,     9,
       9,     0,     9,     0,     2,     0,     5,     0,     2,     4,
       4,     1,     1,     1,     1,     1,     0,    12,     0,    15,
       0,    16,     0,    18,     0,    18,     1,     2,     1,     1,
       1,     1,     1,     8,     8,    10,    10,     5,     5,     0,
       2,     1,     2,     1,     1,     1,     1,     8,     8,    10,
      10,     0,     2,    12,    12,    10,     9,     8,    13,    13,
      11,    10,     1,     1,     1,     2,     3,     6,     6,     1,
       1,     0,     2,     2,     8,     8,     1,     0,     6,     1,
       0,     1,     2,     0,     9,     1,     0,     1,     2,     4,
       5,     1,     0,     1,     1,     1,     2,     2,     2,     2,
       2,     2,     2,     2,     2
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     5,     0,     2,    16,     0,     3,     0,     4,     0,
       1,     0,     0,     0,     9,   111,   112,   113,   114,   115,
      60,     0,     0,     0,    11,     0,    51,     0,     0,    53,
      61,    62,    63,    64,    65,    56,    57,     0,    15,   164,
     171,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    52,     0,    55,    59,     0,     0,   165,     0,     0,
       0,     0,     0,    29,    21,    22,    23,   162,   163,     0,
       0,     0,     0,   193,   194,   195,     0,     0,     0,     0,
       0,   169,   170,     0,     0,     0,   166,   172,   173,    17,
       0,     0,     0,     0,     0,    30,     0,     0,     0,   192,
     196,   197,   198,   199,   200,   201,   202,   203,   204,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    18,
       0,     0,     0,     0,     0,     0,    32,     0,     0,     0,
     191,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    34,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     190,   167,   168,     0,     0,    20,    19,     0,     0,     0,
       0,     0,     0,     0,    43,    35,    36,    37,     0,     0,
       0,     0,    73,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    24,     0,     0,    31,     0,     0,     0,
      45,     0,     0,     0,     0,    76,     0,    70,     0,     0,
       0,     0,     0,     0,     0,    26,    25,    28,    27,    33,
       0,     0,     0,    48,     0,     0,     0,   116,     0,     0,
       0,     0,     0,     0,    75,    77,     0,    79,    80,    81,
      82,    83,    84,    88,    87,    86,    91,     0,    69,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      13,     0,     0,     0,   139,     0,     0,     0,     0,     0,
       0,     0,     0,    74,    78,    90,     0,    68,     0,    71,
      72,   175,   174,     0,    41,    42,     0,     0,     0,     0,
      12,     0,   194,     0,     0,     0,     0,     0,     0,     0,
     139,   126,     0,   128,   129,   130,   131,   132,     0,     0,
       0,     0,     0,     0,     0,     0,    66,    67,    38,     0,
      44,     0,     0,   177,     0,     0,     0,   118,     0,     0,
       0,     0,     0,     0,     0,     0,   117,   127,   140,     0,
       0,     0,     0,     0,     0,     0,     0,    39,     0,    47,
      46,     0,     7,   176,     0,   120,     0,   139,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   101,     0,     0,     0,   139,     0,   139,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   107,    40,     0,   124,
     139,   122,   119,     0,     0,     0,     0,     0,     0,   137,
     138,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   103,   107,   180,   151,   121,   151,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    85,     0,
      98,     0,     0,     0,     0,   108,     0,     0,   179,   181,
       0,     0,     0,     0,   151,   141,     0,   144,   143,   146,
     145,   151,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    94,     0,     0,   100,    99,     0,     0,     0,   102,
     104,     0,   178,   182,     0,     0,     0,     0,     0,     0,
       0,     0,   125,   142,   152,   123,   133,   134,   157,     0,
       0,     0,     0,     0,    92,    93,     0,   194,     0,   110,
     109,   105,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   156,     0,     0,     0,     0,     0,     0,    97,     0,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     155,   161,     0,   135,   136,    95,    96,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   160,   106,   183,
       0,     0,     0,     0,     0,     0,     0,     0,   186,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   185,
     187,   147,   148,     0,     0,     0,     0,     0,     0,     0,
     184,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   149,   150,   189,     0,     0,     0,     0,
     153,   154,     0,     0,   158,   159
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     4,     5,     6,     7,   289,     8,     9,
      12,    43,    63,    64,    65,    66,    95,   126,   149,   174,
     175,   176,   177,   200,   223,   260,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,   205,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   396,   444,   480,   530,   421,   422,
      14,    15,   264,    16,   357,    17,   377,    18,   426,    19,
     424,   300,   301,   302,   454,   455,   456,   457,   458,   303,
     304,   305,   459,   460,   306,   307,    69,    38,    39,    40,
      83,    58,    87,    88,   352,   353,   447,   448,   449,   568,
     578,   579,   580,    53,   131,    75,    76
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -445
static const yytype_int16 yypact[] =
{
      15,  -445,    27,  -445,    32,    61,  -445,   189,  -445,    67,
    -445,    46,    86,    30,  -445,  -445,  -445,  -445,  -445,  -445,
    -445,    52,   120,   136,  -445,   147,  -445,    75,    61,  -445,
    -445,  -445,  -445,  -445,  -445,  -445,  -445,   144,    67,  -445,
    -445,   109,   184,   114,    47,    84,   124,    44,    44,    44,
      44,  -445,    80,  -445,  -445,    29,    29,  -445,    -4,    92,
     139,   143,   228,   135,  -445,  -445,  -445,  -445,  -445,   157,
     164,   169,   173,  -445,  -445,   211,    44,    44,    44,    44,
     180,  -445,  -445,    44,    44,   241,  -445,  -445,  -445,  -445,
      44,     4,    44,    44,   255,   153,   186,   195,    44,   196,
    -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,    44,
      44,   191,   204,   206,   168,   174,    44,    44,    44,  -445,
      44,    44,    44,    44,    44,   192,   210,   236,   102,    44,
    -445,   208,    44,   115,    44,    44,   209,   224,   225,    44,
      44,   230,   229,    44,    44,    44,    44,    44,   242,   260,
      44,    44,    44,   334,   281,    44,   349,   133,    44,    44,
    -445,  -445,  -445,    44,    44,  -445,  -445,   357,    -1,    44,
      44,   291,    44,   315,   356,  -445,  -445,  -445,    44,    44,
      44,   328,  -445,    44,   331,   383,   140,   384,   393,    44,
      44,   351,   350,  -445,   354,   353,  -445,   358,    44,   355,
     378,    44,    44,    44,   359,   294,   401,  -445,   360,   405,
     406,    47,   407,    44,    44,  -445,  -445,  -445,  -445,  -445,
      44,   299,   364,   386,    44,    44,   411,  -445,   279,   280,
     367,   297,   368,   369,   294,  -445,    75,  -445,  -445,  -445,
    -445,  -445,  -445,  -445,  -445,  -445,  -445,    47,  -445,   372,
     414,   375,   374,   379,   380,    44,   381,   382,   422,   298,
     412,    44,    60,   385,    41,    44,    44,    44,    44,    44,
      44,    44,    47,  -445,  -445,  -445,   396,  -445,   395,  -445,
    -445,  -445,  -445,     8,  -445,  -445,   397,   423,   427,   151,
    -445,    44,   398,    44,   400,   301,   402,   403,   302,   305,
     155,  -445,    75,  -445,  -445,  -445,  -445,  -445,    44,    44,
      44,    44,    44,    44,    44,   408,  -445,  -445,  -445,    10,
    -445,   409,   410,   415,    47,   404,   446,  -445,    44,    44,
      44,    44,    44,    44,    44,    44,  -445,  -445,  -445,    44,
      44,    44,    44,    44,    44,    44,   413,  -445,    44,  -445,
    -445,   424,  -445,  -445,   416,  -445,   425,    41,    44,    44,
      44,    44,    44,    44,    44,    44,    44,    44,    44,    44,
      44,    44,   207,  -445,   426,   428,   430,    41,   431,   178,
      44,    44,    44,    44,    44,    44,   429,   432,    44,    44,
      44,    44,   452,   453,   457,   470,   320,  -445,   434,  -445,
     222,  -445,  -445,    44,    44,   226,    44,    44,    44,  -445,
    -445,    44,    44,    44,    44,   442,    47,   443,   459,    44,
      44,  -445,   320,   449,   108,  -445,   108,    44,    44,   490,
     489,    44,    44,    44,    47,     5,    44,    44,  -445,   448,
    -445,   447,    44,    44,   -12,  -445,   450,   460,   449,  -445,
     321,   326,   327,   335,   218,  -445,    75,  -445,  -445,  -445,
    -445,   251,   461,   468,   471,   387,   492,    44,    44,   463,
     472,  -445,    44,    79,  -445,  -445,   479,   480,   451,  -445,
    -445,   525,  -445,  -445,    44,    44,    44,    44,    44,    44,
      44,    44,  -445,  -445,  -445,  -445,  -445,  -445,  -445,   482,
     529,   392,    44,    44,  -445,  -445,    47,   484,   531,  -445,
    -445,  -445,   530,    44,    44,    44,    44,    44,    44,    44,
      44,  -445,   491,   493,   538,   496,   495,   498,  -445,   497,
     320,   499,    44,    44,    44,    44,    44,    44,    44,    44,
    -445,  -445,   500,  -445,  -445,  -445,  -445,   501,   503,    44,
      44,    44,    44,    44,    44,    44,    44,  -445,  -445,  -445,
      44,    44,    44,    44,    44,    44,    44,    44,   515,   505,
     504,    44,    44,    44,    44,    44,    44,   506,   507,   515,
    -445,  -445,  -445,   550,   552,    44,    44,    44,    44,   553,
    -445,  -445,   554,   555,   556,   557,   516,   517,   518,    47,
     561,   560,   563,  -445,  -445,  -445,   522,   521,    47,   568,
    -445,  -445,   526,   523,  -445,  -445
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,   316,  -445,
    -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,
    -445,  -445,  -445,  -445,  -445,  -445,   286,  -445,   558,  -445,
    -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,
    -445,  -445,   343,  -445,  -445,  -445,  -445,  -445,  -445,  -445,
    -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -416,  -445,
     551,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,
    -445,  -336,  -280,  -445,   152,  -444,  -445,  -445,  -445,  -445,
    -445,  -445,  -445,  -445,  -445,  -445,  -207,  -445,   542,  -445,
     528,  -445,  -445,  -445,  -445,  -445,  -445,  -445,   134,  -445,
    -445,  -445,     2,  -231,  -445,   -47,   -48
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -90
static const yytype_int16 yytable[] =
{
      77,    78,    79,   192,   251,   275,   445,    73,    74,   470,
     493,    73,    74,    73,    74,   -10,     1,   493,   478,    85,
     337,   379,    -6,    -6,   -10,   -10,   -10,    10,   109,   110,
     111,   112,    -8,    81,    82,   114,   115,   -14,   479,    11,
     276,   400,   118,   120,   121,   122,    86,    73,    74,   193,
     129,    67,   -10,    68,   119,   471,   318,   295,   347,   296,
     297,   132,   133,    73,   292,   315,   298,   299,   139,   140,
     141,   338,   142,   143,   144,   145,   146,    44,    13,    45,
     152,   153,    73,   507,   155,   157,   158,   159,    70,    37,
      71,   163,   164,    41,    42,   167,   168,   169,   170,   337,
     171,    46,   178,   179,   180,    73,    74,   183,   151,   186,
     187,   188,    52,    59,   547,   189,   190,   354,    73,    74,
     337,   156,   194,   195,   450,   197,   451,   452,    72,    80,
     201,   202,   203,   453,    62,   206,    73,    74,   210,   185,
      89,   213,   214,    73,    74,    90,   209,   -49,    20,    91,
     220,   -50,    20,   224,   225,   226,    21,    22,    23,    94,
      21,    22,    23,    96,   -58,   253,   254,    47,   -58,    48,
      97,   295,   255,   296,   297,    98,   261,   262,   -49,    99,
     298,   299,   -50,    49,   -54,    50,   113,   125,   -54,   -50,
      20,    55,   127,    56,   295,   134,   296,   297,    21,    22,
      23,   128,   130,   298,   299,   336,   -58,   283,   135,   439,
      73,    74,   136,   394,   291,   293,   137,   308,   309,   310,
     311,   312,   313,   314,   138,   494,   -54,   469,   402,    73,
      74,    60,   429,    61,   450,   319,   451,   452,   295,   147,
     296,   297,   150,   453,   324,   148,   326,   298,   299,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   154,   160,
     339,   340,   341,   342,   343,   344,   345,   450,   492,   451,
     452,   348,   425,   161,   162,    92,   453,    93,   165,   166,
     358,   359,   360,   361,   362,   363,   364,   365,   116,   172,
     117,   366,   367,   368,   369,   370,   173,   371,   372,   527,
     374,   495,   123,   256,   124,   257,   228,   229,   230,   231,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   391,   392,   232,   393,   395,   265,   267,   266,   268,
     182,   -89,   403,   404,   405,   406,   407,   408,   181,   196,
     411,   412,   413,   414,   270,   287,   271,   288,   328,   332,
     329,   333,   334,   184,   335,   427,   428,   430,   431,   432,
     433,   191,   198,   434,   435,   436,   437,   419,   484,   420,
     485,   442,   443,   486,   488,   487,   489,   199,   204,   462,
     463,   207,   490,   466,   491,   467,   468,   208,   211,   472,
     473,   499,   606,   500,   476,   477,   523,   212,   524,   215,
     216,   612,   217,   218,   221,   222,   219,   247,   227,   249,
     248,   252,   250,   258,   259,   263,   269,   272,   278,   273,
     502,   503,   277,   279,   280,   506,   508,   281,   286,   321,
     282,   284,   285,   322,   -14,   294,   513,   514,   515,   516,
     517,   518,   519,   520,   316,   317,   351,   320,   325,   327,
     356,   330,   331,   355,   525,   526,   415,   349,   346,   416,
     350,   417,   373,   441,   376,   532,   533,   534,   535,   536,
     537,   538,   539,   375,   397,   378,   418,   409,   398,   399,
     401,   446,   410,   423,   549,   550,   551,   552,   553,   554,
     555,   556,   438,   440,   464,   465,   474,   475,   501,   481,
     511,   560,   561,   562,   563,   564,   565,   566,   567,   496,
     482,   504,   569,   570,   571,   572,   573,   574,   497,   575,
     576,   498,   505,   583,   584,   585,   586,   509,   587,   588,
     510,   512,   521,   522,   528,   529,   531,   594,   595,   596,
     597,   540,   542,   541,   543,   544,   545,   546,   577,   548,
     557,   558,   559,   581,   582,   589,   592,   590,   593,   598,
     599,   600,   601,   602,   603,   607,   608,   604,   605,   609,
     610,   611,   613,   615,   614,   323,   290,   274,   461,    54,
      57,   591,   483,    51,    84
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-445)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_uint16 yycheck[] =
{
      48,    49,    50,     4,   211,   236,   422,     3,     4,     4,
     454,     3,     4,     3,     4,     0,     1,   461,    30,    23,
     300,   357,     7,     8,     9,    10,    11,     0,    76,    77,
      78,    79,    17,     4,     5,    83,    84,    22,    50,     7,
     247,   377,    90,    91,    92,    93,    50,     3,     4,    50,
      98,     4,    37,     6,    50,    50,    48,    16,    48,    18,
      19,   109,   110,     3,     4,   272,    25,    26,   116,   117,
     118,   302,   120,   121,   122,   123,   124,    47,    17,    49,
     128,   129,     3,     4,   132,   133,   134,   135,     4,    22,
       6,   139,   140,    47,     8,   143,   144,   145,   146,   379,
     147,    49,   150,   151,   152,     3,     4,   155,     6,   157,
     158,   159,    37,     4,   530,   163,   164,   324,     3,     4,
     400,     6,   169,   170,    16,   172,    18,    19,     4,    49,
     178,   179,   180,    25,    20,   183,     3,     4,   186,     6,
      48,   189,   190,     3,     4,     6,     6,     0,     1,     6,
     198,     0,     1,   201,   202,   203,     9,    10,    11,    24,
       9,    10,    11,     6,    17,   213,   214,    47,    17,    49,
       6,    16,   220,    18,    19,     6,   224,   225,    31,     6,
      25,    26,    31,    47,    37,    49,     6,    34,    37,     0,
       1,    47,     6,    49,    16,     4,    18,    19,     9,    10,
      11,     6,     6,    25,    26,    50,    17,   255,     4,   416,
       3,     4,     6,     6,   261,   262,    48,   265,   266,   267,
     268,   269,   270,   271,    50,   456,    37,   434,    50,     3,
       4,    47,     6,    49,    16,   283,    18,    19,    16,    47,
      18,    19,     6,    25,   291,    35,   293,    25,    26,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    50,    50,
     308,   309,   310,   311,   312,   313,   314,    16,    50,    18,
      19,   319,    50,    49,    49,    47,    25,    49,    48,    50,
     328,   329,   330,   331,   332,   333,   334,   335,    47,    47,
      49,   339,   340,   341,   342,   343,    36,   344,   345,   506,
     348,    50,    47,     4,    49,     6,    12,    13,    14,    15,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,    29,   371,   372,    47,    47,    49,    49,
      49,    37,   380,   381,   382,   383,   384,   385,     4,    48,
     388,   389,   390,   391,    47,    47,    49,    49,    47,    47,
      49,    49,    47,     4,    49,   403,   404,   405,   406,   407,
     408,     4,    47,   411,   412,   413,   414,    47,    47,    49,
      49,   419,   420,    47,    47,    49,    49,    21,    50,   427,
     428,    50,    47,   431,    49,   432,   433,     4,     4,   436,
     437,     4,   599,     6,   442,   443,     4,     4,     6,    48,
      50,   608,    48,    50,    49,    27,    48,     6,    49,     4,
      50,     4,     6,    49,    28,     4,    49,    49,     4,    50,
     467,   468,    50,    48,    50,   472,   473,    48,     6,     6,
      50,    50,    50,     6,    22,    50,   484,   485,   486,   487,
     488,   489,   490,   491,    48,    50,    31,    50,    50,    49,
       4,    49,    49,    49,   502,   503,     4,    48,    50,     6,
      50,     4,    49,     4,    48,   513,   514,   515,   516,   517,
     518,   519,   520,    49,    48,    50,     6,    48,    50,    49,
      49,    32,    50,    49,   532,   533,   534,   535,   536,   537,
     538,   539,    50,    50,     4,     6,    48,    50,     6,    49,
      49,   549,   550,   551,   552,   553,   554,   555,   556,    48,
      50,    48,   560,   561,   562,   563,   564,   565,    50,   566,
     567,    50,    50,   571,   572,   573,   574,    48,   575,   576,
      50,     6,    50,     4,    50,     4,     6,   585,   586,   587,
     588,    50,     4,    50,    48,    50,    48,    50,    33,    50,
      50,    50,    49,    48,    50,    49,     6,    50,     6,     6,
       6,     6,     6,     6,    48,     4,     6,    50,    50,     6,
      48,    50,     4,    50,    48,   289,   260,   234,   426,    28,
      38,   579,   448,    25,    56
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    52,    53,    54,    55,    56,    57,    59,    60,
       0,     7,    61,    17,   111,   112,   114,   116,   118,   120,
       1,     9,    10,    11,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    22,   138,   139,
     140,    47,     8,    62,    47,    49,    49,    47,    49,    47,
      49,    79,    37,   154,   111,    47,    49,   139,   142,     4,
      47,    49,    20,    63,    64,    65,    66,     4,     6,   137,
       4,     6,     4,     3,     4,   156,   157,   157,   157,   157,
      49,     4,     5,   141,   141,    23,    50,   143,   144,    48,
       6,     6,    47,    49,    24,    67,     6,     6,     6,     6,
      38,    39,    40,    41,    42,    43,    44,    45,    46,   157,
     157,   157,   157,     6,   157,   157,    47,    49,   157,    50,
     157,   157,   157,    47,    49,    34,    68,     6,     6,   157,
       6,   155,   157,   157,     4,     4,     6,    48,    50,   157,
     157,   157,   157,   157,   157,   157,   157,    47,    35,    69,
       6,     6,   157,   157,    50,   157,     6,   157,   157,   157,
      50,    49,    49,   157,   157,    48,    50,   157,   157,   157,
     157,   156,    47,    36,    70,    71,    72,    73,   157,   157,
     157,     4,    49,   157,     4,     6,   157,   157,   157,   157,
     157,     4,     4,    50,   156,   156,    48,   156,    47,    21,
      74,   157,   157,   157,    50,    90,   157,    50,     4,     6,
     157,     4,     4,   157,   157,    48,    50,    48,    50,    48,
     157,    49,    27,    75,   157,   157,   157,    49,    12,    13,
      14,    15,    29,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,     6,    50,     4,
       6,   137,     4,   157,   157,   157,     4,     6,    49,    28,
      76,   157,   157,     4,   113,    47,    49,    47,    49,    49,
      47,    49,    49,    50,    93,   154,   137,    50,     4,    48,
      50,    48,    50,   157,    50,    50,     6,    47,    49,    58,
      59,   156,     4,   156,    50,    16,    18,    19,    25,    26,
     122,   123,   124,   130,   131,   132,   135,   136,   157,   157,
     157,   157,   157,   157,   157,   137,    48,    50,    48,   157,
      50,     6,     6,    77,   156,    50,   156,    49,    47,    49,
      49,    49,    47,    49,    47,    49,    50,   123,   154,   157,
     157,   157,   157,   157,   157,   157,    50,    48,   157,    48,
      50,    31,   145,   146,   137,    49,     4,   115,   157,   157,
     157,   157,   157,   157,   157,   157,   157,   157,   157,   157,
     157,   156,   156,    49,   157,    49,    48,   117,    50,   122,
     157,   157,   157,   157,   157,   157,   157,   157,   157,   157,
     157,   157,   157,   156,     6,   156,   105,    48,    50,    49,
     122,    49,    50,   157,   157,   157,   157,   157,   157,    48,
      50,   157,   157,   157,   157,     4,     6,     4,     6,    47,
      49,   109,   110,    49,   121,    50,   119,   157,   157,     6,
     157,   157,   157,   157,   157,   157,   157,   157,    50,   137,
      50,     4,   157,   157,   106,   109,    32,   147,   148,   149,
      16,    18,    19,    25,   125,   126,   127,   128,   129,   133,
     134,   125,   157,   157,     4,     6,   157,   156,   156,   137,
       4,    50,   156,   156,    48,    50,   157,   157,    30,    50,
     107,    49,    50,   149,    47,    49,    47,    49,    47,    49,
      47,    49,    50,   126,   154,    50,    48,    50,    50,     4,
       6,     6,   156,   156,    48,    50,   156,     4,   156,    48,
      50,    49,     6,   157,   157,   157,   157,   157,   157,   157,
     157,    50,     4,     4,     6,   157,   157,   137,    50,     4,
     108,     6,   157,   157,   157,   157,   157,   157,   157,   157,
      50,    50,     4,    48,    50,    48,    50,   109,    50,   157,
     157,   157,   157,   157,   157,   157,   157,    50,    50,    49,
     157,   157,   157,   157,   157,   157,   157,   157,   150,   157,
     157,   157,   157,   157,   157,   156,   156,    33,   151,   152,
     153,    48,    50,   157,   157,   157,   157,   156,   156,    49,
      50,   153,     6,     6,   157,   157,   157,   157,     6,     6,
       6,     6,     6,    48,    50,    50,   137,     4,     6,     6,
      48,    50,   137,     4,    48,    50
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

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
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
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
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
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
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
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

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YYUSE (yytype);
}




/* The lookahead symbol.  */
int yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

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
      yychar = YYLEX;
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
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 5:
/* Line 1787 of yacc.c  */
#line 140 "parse_y.y"
    { YYABORT; }
    break;

  case 6:
/* Line 1787 of yacc.c  */
#line 164 "parse_y.y"
    {
					/* reset flags for 'used layers';
					 * init font and data pointers
					 */
				int	i;

				if (!yyPCB)
				{
					Message("illegal fileformat\n");
					YYABORT;
				}
				for (i = 0; i < MAX_LAYER + 2; i++)
					LayerFlag[i] = false;
				yyFont = &yyPCB->Font;
				yyData = yyPCB->Data;
				yyData->pcb = yyPCB;
				yyData->LayerN = 0;
				layer_group_string = NULL;
			}
    break;

  case 7:
/* Line 1787 of yacc.c  */
#line 196 "parse_y.y"
    {
			  PCBTypePtr pcb_save = PCB;

			  if (layer_group_string == NULL)
			    layer_group_string = Settings.Groups;
			  CreateNewPCBPost (yyPCB, 0);
			  if (ParseGroupString(layer_group_string, &yyPCB->LayerGroups, yyData->LayerN))
			    {
			      Message("illegal layer-group string\n");
			      YYABORT;
			    }
			/* initialize the polygon clipping now since
			 * we didn't know the layer grouping before.
			 */
			PCB = yyPCB;
			ALLPOLYGON_LOOP (yyData);
			{
			  InitClip (yyData, layer, polygon);
			}
			ENDALL_LOOP;
			PCB = pcb_save;
			}
    break;

  case 8:
/* Line 1787 of yacc.c  */
#line 219 "parse_y.y"
    { PreLoadElementPCB ();
		    layer_group_string = NULL; }
    break;

  case 9:
/* Line 1787 of yacc.c  */
#line 222 "parse_y.y"
    { LayerFlag[0] = true;
		    LayerFlag[1] = true;
		    yyData->LayerN = 2;
		    PostLoadElementPCB ();
		  }
    break;

  case 10:
/* Line 1787 of yacc.c  */
#line 230 "parse_y.y"
    {
					/* reset flags for 'used layers';
					 * init font and data pointers
					 */
				int	i;

				if (!yyData || !yyFont)
				{
					Message("illegal fileformat\n");
					YYABORT;
				}
				for (i = 0; i < MAX_LAYER + 2; i++)
					LayerFlag[i] = false;
				yyData->LayerN = 0;
			}
    break;

  case 14:
/* Line 1787 of yacc.c  */
#line 255 "parse_y.y"
    {
					/* mark all symbols invalid */
				int	i;

				if (!yyFont)
				{
					Message("illegal fileformat\n");
					YYABORT;
				}
				yyFont->Valid = false;
				for (i = 0; i <= MAX_FONTPOSITION; i++)
					free (yyFont->Symbol[i].Line);
				bzero(yyFont->Symbol, sizeof(yyFont->Symbol));
			}
    break;

  case 15:
/* Line 1787 of yacc.c  */
#line 270 "parse_y.y"
    {
				yyFont->Valid = true;
		  		SetFontInfo(yyFont);
			}
    break;

  case 17:
/* Line 1787 of yacc.c  */
#line 298 "parse_y.y"
    {
  if (check_file_version ((yyvsp[(3) - (4)].integer)) != 0)
    {
      YYABORT;
    }
}
    break;

  case 18:
/* Line 1787 of yacc.c  */
#line 328 "parse_y.y"
    {
				yyPCB->Name = (yyvsp[(3) - (4)].string);
				yyPCB->MaxWidth = MAX_COORD;
				yyPCB->MaxHeight = MAX_COORD;
			}
    break;

  case 19:
/* Line 1787 of yacc.c  */
#line 334 "parse_y.y"
    {
				yyPCB->Name = (yyvsp[(3) - (6)].string);
				yyPCB->MaxWidth = OU ((yyvsp[(4) - (6)].measure));
				yyPCB->MaxHeight = OU ((yyvsp[(5) - (6)].measure));
			}
    break;

  case 20:
/* Line 1787 of yacc.c  */
#line 340 "parse_y.y"
    {
				yyPCB->Name = (yyvsp[(3) - (6)].string);
				yyPCB->MaxWidth = NU ((yyvsp[(4) - (6)].measure));
				yyPCB->MaxHeight = NU ((yyvsp[(5) - (6)].measure));
			}
    break;

  case 24:
/* Line 1787 of yacc.c  */
#line 374 "parse_y.y"
    {
				yyPCB->Grid = OU ((yyvsp[(3) - (6)].measure));
				yyPCB->GridOffsetX = OU ((yyvsp[(4) - (6)].measure));
				yyPCB->GridOffsetY = OU ((yyvsp[(5) - (6)].measure));
			}
    break;

  case 25:
/* Line 1787 of yacc.c  */
#line 382 "parse_y.y"
    {
				yyPCB->Grid = OU ((yyvsp[(3) - (7)].measure));
				yyPCB->GridOffsetX = OU ((yyvsp[(4) - (7)].measure));
				yyPCB->GridOffsetY = OU ((yyvsp[(5) - (7)].measure));
				if ((yyvsp[(6) - (7)].integer))
					Settings.DrawGrid = true;
				else
					Settings.DrawGrid = false;
			}
    break;

  case 26:
/* Line 1787 of yacc.c  */
#line 395 "parse_y.y"
    {
				yyPCB->Grid = NU ((yyvsp[(3) - (7)].measure));
				yyPCB->GridOffsetX = NU ((yyvsp[(4) - (7)].measure));
				yyPCB->GridOffsetY = NU ((yyvsp[(5) - (7)].measure));
				if ((yyvsp[(6) - (7)].integer))
					Settings.DrawGrid = true;
				else
					Settings.DrawGrid = false;
			}
    break;

  case 27:
/* Line 1787 of yacc.c  */
#line 427 "parse_y.y"
    {
				yyPCB->CursorX = OU ((yyvsp[(3) - (6)].measure));
				yyPCB->CursorY = OU ((yyvsp[(4) - (6)].measure));
				yyPCB->Zoom = (yyvsp[(5) - (6)].number)*2;
			}
    break;

  case 28:
/* Line 1787 of yacc.c  */
#line 433 "parse_y.y"
    {
				yyPCB->CursorX = NU ((yyvsp[(3) - (6)].measure));
				yyPCB->CursorY = NU ((yyvsp[(4) - (6)].measure));
				yyPCB->Zoom = (yyvsp[(5) - (6)].number);
			}
    break;

  case 31:
/* Line 1787 of yacc.c  */
#line 457 "parse_y.y"
    {
				/* Read in cmil^2 for now; in future this should be a noop. */
				yyPCB->IsleArea = MIL_TO_COORD (MIL_TO_COORD ((yyvsp[(3) - (4)].number)) / 100.0) / 100.0;
			}
    break;

  case 33:
/* Line 1787 of yacc.c  */
#line 484 "parse_y.y"
    {
				yyPCB->ThermScale = (yyvsp[(3) - (4)].number);
			}
    break;

  case 38:
/* Line 1787 of yacc.c  */
#line 523 "parse_y.y"
    {
				yyPCB->Bloat = NU ((yyvsp[(3) - (6)].measure));
				yyPCB->Shrink = NU ((yyvsp[(4) - (6)].measure));
				yyPCB->minWid = NU ((yyvsp[(5) - (6)].measure));
				yyPCB->minRing = NU ((yyvsp[(5) - (6)].measure));
			}
    break;

  case 39:
/* Line 1787 of yacc.c  */
#line 533 "parse_y.y"
    {
				yyPCB->Bloat = NU ((yyvsp[(3) - (7)].measure));
				yyPCB->Shrink = NU ((yyvsp[(4) - (7)].measure));
				yyPCB->minWid = NU ((yyvsp[(5) - (7)].measure));
				yyPCB->minSlk = NU ((yyvsp[(6) - (7)].measure));
				yyPCB->minRing = NU ((yyvsp[(5) - (7)].measure));
			}
    break;

  case 40:
/* Line 1787 of yacc.c  */
#line 544 "parse_y.y"
    {
				yyPCB->Bloat = NU ((yyvsp[(3) - (9)].measure));
				yyPCB->Shrink = NU ((yyvsp[(4) - (9)].measure));
				yyPCB->minWid = NU ((yyvsp[(5) - (9)].measure));
				yyPCB->minSlk = NU ((yyvsp[(6) - (9)].measure));
				yyPCB->minDrill = NU ((yyvsp[(7) - (9)].measure));
				yyPCB->minRing = NU ((yyvsp[(8) - (9)].measure));
			}
    break;

  case 41:
/* Line 1787 of yacc.c  */
#line 571 "parse_y.y"
    {
				yyPCB->Flags = MakeFlags ((yyvsp[(3) - (4)].integer) & PCB_FLAGS);
			}
    break;

  case 42:
/* Line 1787 of yacc.c  */
#line 575 "parse_y.y"
    {
			  yyPCB->Flags = string_to_pcbflags ((yyvsp[(3) - (4)].string), yyerror);
			}
    break;

  case 44:
/* Line 1787 of yacc.c  */
#line 607 "parse_y.y"
    {
			  layer_group_string = (yyvsp[(3) - (4)].string);
			}
    break;

  case 46:
/* Line 1787 of yacc.c  */
#line 655 "parse_y.y"
    {
				if (ParseRouteString((yyvsp[(3) - (4)].string), &yyPCB->RouteStyle[0], "mil"))
				{
					Message("illegal route-style string\n");
					YYABORT;
				}
			}
    break;

  case 47:
/* Line 1787 of yacc.c  */
#line 663 "parse_y.y"
    {
				if (ParseRouteString((yyvsp[(3) - (4)].string), &yyPCB->RouteStyle[0], "cmil"))
				{
					Message("illegal route-style string\n");
					YYABORT;
				}
			}
    break;

  case 54:
/* Line 1787 of yacc.c  */
#line 685 "parse_y.y"
    { attr_list = & yyPCB->Attributes; }
    break;

  case 58:
/* Line 1787 of yacc.c  */
#line 689 "parse_y.y"
    {
					/* clear pointer to force memory allocation by 
					 * the appropriate subroutine
					 */
				yyElement = NULL;
			}
    break;

  case 60:
/* Line 1787 of yacc.c  */
#line 696 "parse_y.y"
    { YYABORT; }
    break;

  case 66:
/* Line 1787 of yacc.c  */
#line 741 "parse_y.y"
    {
				CreateNewVia(yyData, NU ((yyvsp[(3) - (11)].measure)), NU ((yyvsp[(4) - (11)].measure)), NU ((yyvsp[(5) - (11)].measure)), NU ((yyvsp[(6) - (11)].measure)), NU ((yyvsp[(7) - (11)].measure)),
				                     NU ((yyvsp[(8) - (11)].measure)), (yyvsp[(9) - (11)].string), (yyvsp[(10) - (11)].flagtype));
				free ((yyvsp[(9) - (11)].string));
			}
    break;

  case 67:
/* Line 1787 of yacc.c  */
#line 751 "parse_y.y"
    {
				CreateNewVia(yyData, OU ((yyvsp[(3) - (11)].measure)), OU ((yyvsp[(4) - (11)].measure)), OU ((yyvsp[(5) - (11)].measure)), OU ((yyvsp[(6) - (11)].measure)), OU ((yyvsp[(7) - (11)].measure)), OU ((yyvsp[(8) - (11)].measure)), (yyvsp[(9) - (11)].string),
					OldFlags((yyvsp[(10) - (11)].integer)));
				free ((yyvsp[(9) - (11)].string));
			}
    break;

  case 68:
/* Line 1787 of yacc.c  */
#line 762 "parse_y.y"
    {
				CreateNewVia(yyData, OU ((yyvsp[(3) - (10)].measure)), OU ((yyvsp[(4) - (10)].measure)), OU ((yyvsp[(5) - (10)].measure)), OU ((yyvsp[(6) - (10)].measure)),
					     OU ((yyvsp[(5) - (10)].measure)) + OU((yyvsp[(6) - (10)].measure)), OU ((yyvsp[(7) - (10)].measure)), (yyvsp[(8) - (10)].string), OldFlags((yyvsp[(9) - (10)].integer)));
				free ((yyvsp[(8) - (10)].string));
			}
    break;

  case 69:
/* Line 1787 of yacc.c  */
#line 772 "parse_y.y"
    {
				CreateNewVia(yyData, OU ((yyvsp[(3) - (9)].measure)), OU ((yyvsp[(4) - (9)].measure)), OU ((yyvsp[(5) - (9)].measure)), 2*GROUNDPLANEFRAME,
					OU((yyvsp[(5) - (9)].measure)) + 2*MASKFRAME,  OU ((yyvsp[(6) - (9)].measure)), (yyvsp[(7) - (9)].string), OldFlags((yyvsp[(8) - (9)].integer)));
				free ((yyvsp[(7) - (9)].string));
			}
    break;

  case 70:
/* Line 1787 of yacc.c  */
#line 782 "parse_y.y"
    {
				Coord	hole = (OU((yyvsp[(5) - (8)].measure)) * DEFAULT_DRILLINGHOLE);

					/* make sure that there's enough copper left */
				if (OU((yyvsp[(5) - (8)].measure)) - hole < MIN_PINORVIACOPPER && 
					OU((yyvsp[(5) - (8)].measure)) > MIN_PINORVIACOPPER)
					hole = OU((yyvsp[(5) - (8)].measure)) - MIN_PINORVIACOPPER;

				CreateNewVia(yyData, OU ((yyvsp[(3) - (8)].measure)), OU ((yyvsp[(4) - (8)].measure)), OU ((yyvsp[(5) - (8)].measure)), 2*GROUNDPLANEFRAME,
					OU((yyvsp[(5) - (8)].measure)) + 2*MASKFRAME, hole, (yyvsp[(6) - (8)].string), OldFlags((yyvsp[(7) - (8)].integer)));
				free ((yyvsp[(6) - (8)].string));
			}
    break;

  case 71:
/* Line 1787 of yacc.c  */
#line 818 "parse_y.y"
    {
				CreateNewRat(yyData, NU ((yyvsp[(3) - (10)].measure)), NU ((yyvsp[(4) - (10)].measure)), NU ((yyvsp[(6) - (10)].measure)), NU ((yyvsp[(7) - (10)].measure)), (yyvsp[(5) - (10)].integer), (yyvsp[(8) - (10)].integer),
					Settings.RatThickness, (yyvsp[(9) - (10)].flagtype));
			}
    break;

  case 72:
/* Line 1787 of yacc.c  */
#line 823 "parse_y.y"
    {
				CreateNewRat(yyData, OU ((yyvsp[(3) - (10)].measure)), OU ((yyvsp[(4) - (10)].measure)), OU ((yyvsp[(6) - (10)].measure)), OU ((yyvsp[(7) - (10)].measure)), (yyvsp[(5) - (10)].integer), (yyvsp[(8) - (10)].integer),
					Settings.RatThickness, OldFlags((yyvsp[(9) - (10)].integer)));
			}
    break;

  case 73:
/* Line 1787 of yacc.c  */
#line 854 "parse_y.y"
    {
				if ((yyvsp[(3) - (7)].integer) <= 0 || (yyvsp[(3) - (7)].integer) > MAX_LAYER + 2)
				{
					yyerror("Layernumber out of range");
					YYABORT;
				}
				if (LayerFlag[(yyvsp[(3) - (7)].integer)-1])
				{
					yyerror("Layernumber used twice");
					YYABORT;
				}
				Layer = &yyData->Layer[(yyvsp[(3) - (7)].integer)-1];

					/* memory for name is already allocated */
				Layer->Name = (yyvsp[(4) - (7)].string);
				LayerFlag[(yyvsp[(3) - (7)].integer)-1] = true;
				if (yyData->LayerN + 2 < (yyvsp[(3) - (7)].integer))
				  yyData->LayerN = (yyvsp[(3) - (7)].integer) - 2;
			}
    break;

  case 85:
/* Line 1787 of yacc.c  */
#line 895 "parse_y.y"
    {
				CreateNewPolygonFromRectangle(Layer,
					OU ((yyvsp[(3) - (8)].measure)), OU ((yyvsp[(4) - (8)].measure)), OU ((yyvsp[(3) - (8)].measure)) + OU ((yyvsp[(5) - (8)].measure)), OU ((yyvsp[(4) - (8)].measure)) + OU ((yyvsp[(6) - (8)].measure)), OldFlags((yyvsp[(7) - (8)].integer)));
			}
    break;

  case 89:
/* Line 1787 of yacc.c  */
#line 902 "parse_y.y"
    { attr_list = & Layer->Attributes; }
    break;

  case 92:
/* Line 1787 of yacc.c  */
#line 934 "parse_y.y"
    {
				CreateNewLineOnLayer(Layer, NU ((yyvsp[(3) - (10)].measure)), NU ((yyvsp[(4) - (10)].measure)), NU ((yyvsp[(5) - (10)].measure)), NU ((yyvsp[(6) - (10)].measure)),
				                            NU ((yyvsp[(7) - (10)].measure)), NU ((yyvsp[(8) - (10)].measure)), (yyvsp[(9) - (10)].flagtype));
			}
    break;

  case 93:
/* Line 1787 of yacc.c  */
#line 943 "parse_y.y"
    {
				CreateNewLineOnLayer(Layer, OU ((yyvsp[(3) - (10)].measure)), OU ((yyvsp[(4) - (10)].measure)), OU ((yyvsp[(5) - (10)].measure)), OU ((yyvsp[(6) - (10)].measure)),
						     OU ((yyvsp[(7) - (10)].measure)), OU ((yyvsp[(8) - (10)].measure)), OldFlags((yyvsp[(9) - (10)].integer)));
			}
    break;

  case 94:
/* Line 1787 of yacc.c  */
#line 952 "parse_y.y"
    {
				/* eliminate old-style rat-lines */
			if ((IV ((yyvsp[(8) - (9)].measure)) & RATFLAG) == 0)
				CreateNewLineOnLayer(Layer, OU ((yyvsp[(3) - (9)].measure)), OU ((yyvsp[(4) - (9)].measure)), OU ((yyvsp[(5) - (9)].measure)), OU ((yyvsp[(6) - (9)].measure)), OU ((yyvsp[(7) - (9)].measure)),
					200*GROUNDPLANEFRAME, OldFlags(IV ((yyvsp[(8) - (9)].measure))));
			}
    break;

  case 95:
/* Line 1787 of yacc.c  */
#line 1000 "parse_y.y"
    {
			  CreateNewArcOnLayer(Layer, NU ((yyvsp[(3) - (12)].measure)), NU ((yyvsp[(4) - (12)].measure)), NU ((yyvsp[(5) - (12)].measure)), NU ((yyvsp[(6) - (12)].measure)), (yyvsp[(9) - (12)].number), (yyvsp[(10) - (12)].number),
			                             NU ((yyvsp[(7) - (12)].measure)), NU ((yyvsp[(8) - (12)].measure)), (yyvsp[(11) - (12)].flagtype));
			}
    break;

  case 96:
/* Line 1787 of yacc.c  */
#line 1009 "parse_y.y"
    {
				CreateNewArcOnLayer(Layer, OU ((yyvsp[(3) - (12)].measure)), OU ((yyvsp[(4) - (12)].measure)), OU ((yyvsp[(5) - (12)].measure)), OU ((yyvsp[(6) - (12)].measure)), (yyvsp[(9) - (12)].number), (yyvsp[(10) - (12)].number),
						    OU ((yyvsp[(7) - (12)].measure)), OU ((yyvsp[(8) - (12)].measure)), OldFlags((yyvsp[(11) - (12)].integer)));
			}
    break;

  case 97:
/* Line 1787 of yacc.c  */
#line 1018 "parse_y.y"
    {
				CreateNewArcOnLayer(Layer, OU ((yyvsp[(3) - (11)].measure)), OU ((yyvsp[(4) - (11)].measure)), OU ((yyvsp[(5) - (11)].measure)), OU ((yyvsp[(5) - (11)].measure)), IV ((yyvsp[(8) - (11)].measure)), (yyvsp[(9) - (11)].number),
					OU ((yyvsp[(7) - (11)].measure)), 200*GROUNDPLANEFRAME, OldFlags((yyvsp[(10) - (11)].integer)));
			}
    break;

  case 98:
/* Line 1787 of yacc.c  */
#line 1055 "parse_y.y"
    {
					/* use a default scale of 100% */
				CreateNewText(Layer,yyFont,OU ((yyvsp[(3) - (8)].measure)), OU ((yyvsp[(4) - (8)].measure)), (yyvsp[(5) - (8)].number), 100, (yyvsp[(6) - (8)].string), OldFlags((yyvsp[(7) - (8)].integer)));
				free ((yyvsp[(6) - (8)].string));
			}
    break;

  case 99:
/* Line 1787 of yacc.c  */
#line 1065 "parse_y.y"
    {
				if ((yyvsp[(8) - (9)].integer) & ONSILKFLAG)
				{
					LayerTypePtr lay = &yyData->Layer[yyData->LayerN +
						(((yyvsp[(8) - (9)].integer) & ONSOLDERFLAG) ? SOLDER_LAYER : COMPONENT_LAYER)];

					CreateNewText(lay ,yyFont, OU ((yyvsp[(3) - (9)].measure)), OU ((yyvsp[(4) - (9)].measure)), (yyvsp[(5) - (9)].number), (yyvsp[(6) - (9)].number), (yyvsp[(7) - (9)].string),
						      OldFlags((yyvsp[(8) - (9)].integer)));
				}
				else
					CreateNewText(Layer, yyFont, OU ((yyvsp[(3) - (9)].measure)), OU ((yyvsp[(4) - (9)].measure)), (yyvsp[(5) - (9)].number), (yyvsp[(6) - (9)].number), (yyvsp[(7) - (9)].string),
						      OldFlags((yyvsp[(8) - (9)].integer)));
				free ((yyvsp[(7) - (9)].string));
			}
    break;

  case 100:
/* Line 1787 of yacc.c  */
#line 1083 "parse_y.y"
    {
				/* FIXME: shouldn't know about .f */
				/* I don't think this matters because anything with hi_format
				 * will have the silk on its own layer in the file rather
				 * than using the ONSILKFLAG and having it in a copper layer.
				 * Thus there is no need for anything besides the 'else'
				 * part of this code.
				 */
				if ((yyvsp[(8) - (9)].flagtype).f & ONSILKFLAG)
				{
					LayerTypePtr lay = &yyData->Layer[yyData->LayerN +
						(((yyvsp[(8) - (9)].flagtype).f & ONSOLDERFLAG) ? SOLDER_LAYER : COMPONENT_LAYER)];

					CreateNewText(lay, yyFont, NU ((yyvsp[(3) - (9)].measure)), NU ((yyvsp[(4) - (9)].measure)), (yyvsp[(5) - (9)].number), (yyvsp[(6) - (9)].number), (yyvsp[(7) - (9)].string), (yyvsp[(8) - (9)].flagtype));
				}
				else
					CreateNewText(Layer, yyFont, NU ((yyvsp[(3) - (9)].measure)), NU ((yyvsp[(4) - (9)].measure)), (yyvsp[(5) - (9)].number), (yyvsp[(6) - (9)].number), (yyvsp[(7) - (9)].string), (yyvsp[(8) - (9)].flagtype));
				free ((yyvsp[(7) - (9)].string));
			}
    break;

  case 101:
/* Line 1787 of yacc.c  */
#line 1132 "parse_y.y"
    {
				Polygon = CreateNewPolygon(Layer, (yyvsp[(3) - (5)].flagtype));
			}
    break;

  case 102:
/* Line 1787 of yacc.c  */
#line 1137 "parse_y.y"
    {
				Cardinal contour, contour_start, contour_end;
				bool bad_contour_found = false;
				/* ignore junk */
				for (contour = 0; contour <= Polygon->HoleIndexN; contour++)
				  {
				    contour_start = (contour == 0) ?
						      0 : Polygon->HoleIndex[contour - 1];
				    contour_end = (contour == Polygon->HoleIndexN) ?
						 Polygon->PointN :
						 Polygon->HoleIndex[contour];
				    if (contour_end - contour_start < 3)
				      bad_contour_found = true;
				  }

				if (bad_contour_found)
				  {
				    Message("WARNING parsing file '%s'\n"
					    "    line:        %i\n"
					    "    description: 'ignored polygon (< 3 points in a contour)'\n",
					    yyfilename, yylineno);
				    DestroyObject(yyData, POLYGON_TYPE, Layer, Polygon, Polygon);
				  }
				else
				  {
				    SetPolygonBoundingBox (Polygon);
				    if (!Layer->polygon_tree)
				      Layer->polygon_tree = r_create_tree (NULL, 0, 0);
				    r_insert_entry (Layer->polygon_tree, (BoxType *) Polygon, 0);
				  }
			}
    break;

  case 105:
/* Line 1787 of yacc.c  */
#line 1177 "parse_y.y"
    {
				CreateNewHoleInPolygon (Polygon);
			}
    break;

  case 109:
/* Line 1787 of yacc.c  */
#line 1191 "parse_y.y"
    {
				CreateNewPointInPolygon(Polygon, OU ((yyvsp[(2) - (4)].measure)), OU ((yyvsp[(3) - (4)].measure)));
			}
    break;

  case 110:
/* Line 1787 of yacc.c  */
#line 1195 "parse_y.y"
    {
				CreateNewPointInPolygon(Polygon, NU ((yyvsp[(2) - (4)].measure)), NU ((yyvsp[(3) - (4)].measure)));
			}
    break;

  case 116:
/* Line 1787 of yacc.c  */
#line 1266 "parse_y.y"
    {
				yyElement = CreateNewElement(yyData, yyElement, yyFont, NoFlags(),
					(yyvsp[(3) - (9)].string), (yyvsp[(4) - (9)].string), NULL, OU ((yyvsp[(5) - (9)].measure)), OU ((yyvsp[(6) - (9)].measure)), (yyvsp[(7) - (9)].integer), 100, NoFlags(), false);
				free ((yyvsp[(3) - (9)].string));
				free ((yyvsp[(4) - (9)].string));
				pin_num = 1;
			}
    break;

  case 117:
/* Line 1787 of yacc.c  */
#line 1274 "parse_y.y"
    {
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
    break;

  case 118:
/* Line 1787 of yacc.c  */
#line 1284 "parse_y.y"
    {
				yyElement = CreateNewElement(yyData, yyElement, yyFont, OldFlags((yyvsp[(3) - (12)].integer)),
					(yyvsp[(4) - (12)].string), (yyvsp[(5) - (12)].string), NULL, OU ((yyvsp[(6) - (12)].measure)), OU ((yyvsp[(7) - (12)].measure)), IV ((yyvsp[(8) - (12)].measure)), IV ((yyvsp[(9) - (12)].measure)), OldFlags((yyvsp[(10) - (12)].integer)), false);
				free ((yyvsp[(4) - (12)].string));
				free ((yyvsp[(5) - (12)].string));
				pin_num = 1;
			}
    break;

  case 119:
/* Line 1787 of yacc.c  */
#line 1292 "parse_y.y"
    {
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
    break;

  case 120:
/* Line 1787 of yacc.c  */
#line 1302 "parse_y.y"
    {
				yyElement = CreateNewElement(yyData, yyElement, yyFont, OldFlags((yyvsp[(3) - (13)].integer)),
					(yyvsp[(4) - (13)].string), (yyvsp[(5) - (13)].string), (yyvsp[(6) - (13)].string), OU ((yyvsp[(7) - (13)].measure)), OU ((yyvsp[(8) - (13)].measure)), IV ((yyvsp[(9) - (13)].measure)), IV ((yyvsp[(10) - (13)].measure)), OldFlags((yyvsp[(11) - (13)].integer)), false);
				free ((yyvsp[(4) - (13)].string));
				free ((yyvsp[(5) - (13)].string));
				free ((yyvsp[(6) - (13)].string));
				pin_num = 1;
			}
    break;

  case 121:
/* Line 1787 of yacc.c  */
#line 1311 "parse_y.y"
    {
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
    break;

  case 122:
/* Line 1787 of yacc.c  */
#line 1322 "parse_y.y"
    {
				yyElement = CreateNewElement(yyData, yyElement, yyFont, OldFlags((yyvsp[(3) - (15)].integer)),
					(yyvsp[(4) - (15)].string), (yyvsp[(5) - (15)].string), (yyvsp[(6) - (15)].string), OU ((yyvsp[(7) - (15)].measure)) + OU ((yyvsp[(9) - (15)].measure)), OU ((yyvsp[(8) - (15)].measure)) + OU ((yyvsp[(10) - (15)].measure)),
					(yyvsp[(11) - (15)].number), (yyvsp[(12) - (15)].number), OldFlags((yyvsp[(13) - (15)].integer)), false);
				yyElement->MarkX = OU ((yyvsp[(7) - (15)].measure));
				yyElement->MarkY = OU ((yyvsp[(8) - (15)].measure));
				free ((yyvsp[(4) - (15)].string));
				free ((yyvsp[(5) - (15)].string));
				free ((yyvsp[(6) - (15)].string));
			}
    break;

  case 123:
/* Line 1787 of yacc.c  */
#line 1333 "parse_y.y"
    {
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
    break;

  case 124:
/* Line 1787 of yacc.c  */
#line 1344 "parse_y.y"
    {
				yyElement = CreateNewElement(yyData, yyElement, yyFont, (yyvsp[(3) - (15)].flagtype),
					(yyvsp[(4) - (15)].string), (yyvsp[(5) - (15)].string), (yyvsp[(6) - (15)].string), NU ((yyvsp[(7) - (15)].measure)) + NU ((yyvsp[(9) - (15)].measure)), NU ((yyvsp[(8) - (15)].measure)) + NU ((yyvsp[(10) - (15)].measure)),
					(yyvsp[(11) - (15)].number), (yyvsp[(12) - (15)].number), (yyvsp[(13) - (15)].flagtype), false);
				yyElement->MarkX = NU ((yyvsp[(7) - (15)].measure));
				yyElement->MarkY = NU ((yyvsp[(8) - (15)].measure));
				free ((yyvsp[(4) - (15)].string));
				free ((yyvsp[(5) - (15)].string));
				free ((yyvsp[(6) - (15)].string));
			}
    break;

  case 125:
/* Line 1787 of yacc.c  */
#line 1355 "parse_y.y"
    {
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
    break;

  case 133:
/* Line 1787 of yacc.c  */
#line 1435 "parse_y.y"
    {
				CreateNewLineInElement(yyElement, NU ((yyvsp[(3) - (8)].measure)), NU ((yyvsp[(4) - (8)].measure)), NU ((yyvsp[(5) - (8)].measure)), NU ((yyvsp[(6) - (8)].measure)), NU ((yyvsp[(7) - (8)].measure)));
			}
    break;

  case 134:
/* Line 1787 of yacc.c  */
#line 1440 "parse_y.y"
    {
				CreateNewLineInElement(yyElement, OU ((yyvsp[(3) - (8)].measure)), OU ((yyvsp[(4) - (8)].measure)), OU ((yyvsp[(5) - (8)].measure)), OU ((yyvsp[(6) - (8)].measure)), OU ((yyvsp[(7) - (8)].measure)));
			}
    break;

  case 135:
/* Line 1787 of yacc.c  */
#line 1445 "parse_y.y"
    {
				CreateNewArcInElement(yyElement, NU ((yyvsp[(3) - (10)].measure)), NU ((yyvsp[(4) - (10)].measure)), NU ((yyvsp[(5) - (10)].measure)), NU ((yyvsp[(6) - (10)].measure)), (yyvsp[(7) - (10)].number), (yyvsp[(8) - (10)].number), NU ((yyvsp[(9) - (10)].measure)));
			}
    break;

  case 136:
/* Line 1787 of yacc.c  */
#line 1450 "parse_y.y"
    {
				CreateNewArcInElement(yyElement, OU ((yyvsp[(3) - (10)].measure)), OU ((yyvsp[(4) - (10)].measure)), OU ((yyvsp[(5) - (10)].measure)), OU ((yyvsp[(6) - (10)].measure)), (yyvsp[(7) - (10)].number), (yyvsp[(8) - (10)].number), OU ((yyvsp[(9) - (10)].measure)));
			}
    break;

  case 137:
/* Line 1787 of yacc.c  */
#line 1455 "parse_y.y"
    {
				yyElement->MarkX = NU ((yyvsp[(3) - (5)].measure));
				yyElement->MarkY = NU ((yyvsp[(4) - (5)].measure));
			}
    break;

  case 138:
/* Line 1787 of yacc.c  */
#line 1460 "parse_y.y"
    {
				yyElement->MarkX = OU ((yyvsp[(3) - (5)].measure));
				yyElement->MarkY = OU ((yyvsp[(4) - (5)].measure));
			}
    break;

  case 139:
/* Line 1787 of yacc.c  */
#line 1464 "parse_y.y"
    { attr_list = & yyElement->Attributes; }
    break;

  case 147:
/* Line 1787 of yacc.c  */
#line 1479 "parse_y.y"
    {
				CreateNewLineInElement(yyElement, NU ((yyvsp[(3) - (8)].measure)) + yyElement->MarkX,
					NU ((yyvsp[(4) - (8)].measure)) + yyElement->MarkY, NU ((yyvsp[(5) - (8)].measure)) + yyElement->MarkX,
					NU ((yyvsp[(6) - (8)].measure)) + yyElement->MarkY, NU ((yyvsp[(7) - (8)].measure)));
			}
    break;

  case 148:
/* Line 1787 of yacc.c  */
#line 1485 "parse_y.y"
    {
				CreateNewLineInElement(yyElement, OU ((yyvsp[(3) - (8)].measure)) + yyElement->MarkX,
					OU ((yyvsp[(4) - (8)].measure)) + yyElement->MarkY, OU ((yyvsp[(5) - (8)].measure)) + yyElement->MarkX,
					OU ((yyvsp[(6) - (8)].measure)) + yyElement->MarkY, OU ((yyvsp[(7) - (8)].measure)));
			}
    break;

  case 149:
/* Line 1787 of yacc.c  */
#line 1492 "parse_y.y"
    {
				CreateNewArcInElement(yyElement, NU ((yyvsp[(3) - (10)].measure)) + yyElement->MarkX,
					NU ((yyvsp[(4) - (10)].measure)) + yyElement->MarkY, NU ((yyvsp[(5) - (10)].measure)), NU ((yyvsp[(6) - (10)].measure)), (yyvsp[(7) - (10)].number), (yyvsp[(8) - (10)].number), NU ((yyvsp[(9) - (10)].measure)));
			}
    break;

  case 150:
/* Line 1787 of yacc.c  */
#line 1497 "parse_y.y"
    {
				CreateNewArcInElement(yyElement, OU ((yyvsp[(3) - (10)].measure)) + yyElement->MarkX,
					OU ((yyvsp[(4) - (10)].measure)) + yyElement->MarkY, OU ((yyvsp[(5) - (10)].measure)), OU ((yyvsp[(6) - (10)].measure)), (yyvsp[(7) - (10)].number), (yyvsp[(8) - (10)].number), OU ((yyvsp[(9) - (10)].measure)));
			}
    break;

  case 151:
/* Line 1787 of yacc.c  */
#line 1501 "parse_y.y"
    { attr_list = & yyElement->Attributes; }
    break;

  case 153:
/* Line 1787 of yacc.c  */
#line 1543 "parse_y.y"
    {
				CreateNewPin(yyElement, NU ((yyvsp[(3) - (12)].measure)) + yyElement->MarkX,
					NU ((yyvsp[(4) - (12)].measure)) + yyElement->MarkY, NU ((yyvsp[(5) - (12)].measure)), NU ((yyvsp[(6) - (12)].measure)), NU ((yyvsp[(7) - (12)].measure)), NU ((yyvsp[(8) - (12)].measure)), (yyvsp[(9) - (12)].string),
					(yyvsp[(10) - (12)].string), (yyvsp[(11) - (12)].flagtype));
				free ((yyvsp[(9) - (12)].string));
				free ((yyvsp[(10) - (12)].string));
			}
    break;

  case 154:
/* Line 1787 of yacc.c  */
#line 1555 "parse_y.y"
    {
				CreateNewPin(yyElement, OU ((yyvsp[(3) - (12)].measure)) + yyElement->MarkX,
					OU ((yyvsp[(4) - (12)].measure)) + yyElement->MarkY, OU ((yyvsp[(5) - (12)].measure)), OU ((yyvsp[(6) - (12)].measure)), OU ((yyvsp[(7) - (12)].measure)), OU ((yyvsp[(8) - (12)].measure)), (yyvsp[(9) - (12)].string),
					(yyvsp[(10) - (12)].string), OldFlags((yyvsp[(11) - (12)].integer)));
				free ((yyvsp[(9) - (12)].string));
				free ((yyvsp[(10) - (12)].string));
			}
    break;

  case 155:
/* Line 1787 of yacc.c  */
#line 1567 "parse_y.y"
    {
				CreateNewPin(yyElement, OU ((yyvsp[(3) - (10)].measure)), OU ((yyvsp[(4) - (10)].measure)), OU ((yyvsp[(5) - (10)].measure)), 2*GROUNDPLANEFRAME,
					OU ((yyvsp[(5) - (10)].measure)) + 2*MASKFRAME, OU ((yyvsp[(6) - (10)].measure)), (yyvsp[(7) - (10)].string), (yyvsp[(8) - (10)].string), OldFlags((yyvsp[(9) - (10)].integer)));
				free ((yyvsp[(7) - (10)].string));
				free ((yyvsp[(8) - (10)].string));
			}
    break;

  case 156:
/* Line 1787 of yacc.c  */
#line 1578 "parse_y.y"
    {
				char	p_number[8];

				sprintf(p_number, "%d", pin_num++);
				CreateNewPin(yyElement, OU ((yyvsp[(3) - (9)].measure)), OU ((yyvsp[(4) - (9)].measure)), OU ((yyvsp[(5) - (9)].measure)), 2*GROUNDPLANEFRAME,
					OU ((yyvsp[(5) - (9)].measure)) + 2*MASKFRAME, OU ((yyvsp[(6) - (9)].measure)), (yyvsp[(7) - (9)].string), p_number, OldFlags((yyvsp[(8) - (9)].integer)));

				free ((yyvsp[(7) - (9)].string));
			}
    break;

  case 157:
/* Line 1787 of yacc.c  */
#line 1594 "parse_y.y"
    {
				Coord	hole = OU ((yyvsp[(5) - (8)].measure)) * DEFAULT_DRILLINGHOLE;
				char	p_number[8];

					/* make sure that there's enough copper left */
				if (OU ((yyvsp[(5) - (8)].measure)) - hole < MIN_PINORVIACOPPER && 
					OU ((yyvsp[(5) - (8)].measure)) > MIN_PINORVIACOPPER)
					hole = OU ((yyvsp[(5) - (8)].measure)) - MIN_PINORVIACOPPER;

				sprintf(p_number, "%d", pin_num++);
				CreateNewPin(yyElement, OU ((yyvsp[(3) - (8)].measure)), OU ((yyvsp[(4) - (8)].measure)), OU ((yyvsp[(5) - (8)].measure)), 2*GROUNDPLANEFRAME,
					OU ((yyvsp[(5) - (8)].measure)) + 2*MASKFRAME, hole, (yyvsp[(6) - (8)].string), p_number, OldFlags((yyvsp[(7) - (8)].integer)));
				free ((yyvsp[(6) - (8)].string));
			}
    break;

  case 158:
/* Line 1787 of yacc.c  */
#line 1648 "parse_y.y"
    {
				CreateNewPad(yyElement, NU ((yyvsp[(3) - (13)].measure)) + yyElement->MarkX,
					NU ((yyvsp[(4) - (13)].measure)) + yyElement->MarkY,
					NU ((yyvsp[(5) - (13)].measure)) + yyElement->MarkX,
					NU ((yyvsp[(6) - (13)].measure)) + yyElement->MarkY, NU ((yyvsp[(7) - (13)].measure)), NU ((yyvsp[(8) - (13)].measure)), NU ((yyvsp[(9) - (13)].measure)),
					(yyvsp[(10) - (13)].string), (yyvsp[(11) - (13)].string), (yyvsp[(12) - (13)].flagtype));
				free ((yyvsp[(10) - (13)].string));
				free ((yyvsp[(11) - (13)].string));
			}
    break;

  case 159:
/* Line 1787 of yacc.c  */
#line 1662 "parse_y.y"
    {
				CreateNewPad(yyElement,OU ((yyvsp[(3) - (13)].measure)) + yyElement->MarkX,
					OU ((yyvsp[(4) - (13)].measure)) + yyElement->MarkY, OU ((yyvsp[(5) - (13)].measure)) + yyElement->MarkX,
					OU ((yyvsp[(6) - (13)].measure)) + yyElement->MarkY, OU ((yyvsp[(7) - (13)].measure)), OU ((yyvsp[(8) - (13)].measure)), OU ((yyvsp[(9) - (13)].measure)),
					(yyvsp[(10) - (13)].string), (yyvsp[(11) - (13)].string), OldFlags((yyvsp[(12) - (13)].integer)));
				free ((yyvsp[(10) - (13)].string));
				free ((yyvsp[(11) - (13)].string));
			}
    break;

  case 160:
/* Line 1787 of yacc.c  */
#line 1675 "parse_y.y"
    {
				CreateNewPad(yyElement,OU ((yyvsp[(3) - (11)].measure)),OU ((yyvsp[(4) - (11)].measure)),OU ((yyvsp[(5) - (11)].measure)),OU ((yyvsp[(6) - (11)].measure)),OU ((yyvsp[(7) - (11)].measure)), 2*GROUNDPLANEFRAME,
					OU ((yyvsp[(7) - (11)].measure)) + 2*MASKFRAME, (yyvsp[(8) - (11)].string), (yyvsp[(9) - (11)].string), OldFlags((yyvsp[(10) - (11)].integer)));
				free ((yyvsp[(8) - (11)].string));
				free ((yyvsp[(9) - (11)].string));
			}
    break;

  case 161:
/* Line 1787 of yacc.c  */
#line 1686 "parse_y.y"
    {
				char		p_number[8];

				sprintf(p_number, "%d", pin_num++);
				CreateNewPad(yyElement,OU ((yyvsp[(3) - (10)].measure)),OU ((yyvsp[(4) - (10)].measure)),OU ((yyvsp[(5) - (10)].measure)),OU ((yyvsp[(6) - (10)].measure)),OU ((yyvsp[(7) - (10)].measure)), 2*GROUNDPLANEFRAME,
					OU ((yyvsp[(7) - (10)].measure)) + 2*MASKFRAME, (yyvsp[(8) - (10)].string),p_number, OldFlags((yyvsp[(9) - (10)].integer)));
				free ((yyvsp[(8) - (10)].string));
			}
    break;

  case 162:
/* Line 1787 of yacc.c  */
#line 1696 "parse_y.y"
    { (yyval.flagtype) = OldFlags((yyvsp[(1) - (1)].integer)); }
    break;

  case 163:
/* Line 1787 of yacc.c  */
#line 1697 "parse_y.y"
    { (yyval.flagtype) = string_to_flags ((yyvsp[(1) - (1)].string), yyerror); }
    break;

  case 167:
/* Line 1787 of yacc.c  */
#line 1727 "parse_y.y"
    {
				if ((yyvsp[(3) - (6)].integer) <= 0 || (yyvsp[(3) - (6)].integer) > MAX_FONTPOSITION)
				{
					yyerror("fontposition out of range");
					YYABORT;
				}
				Symbol = &yyFont->Symbol[(yyvsp[(3) - (6)].integer)];
				if (Symbol->Valid)
				{
					yyerror("symbol ID used twice");
					YYABORT;
				}
				Symbol->Valid = true;
				Symbol->Delta = NU ((yyvsp[(4) - (6)].measure));
			}
    break;

  case 168:
/* Line 1787 of yacc.c  */
#line 1743 "parse_y.y"
    {
				if ((yyvsp[(3) - (6)].integer) <= 0 || (yyvsp[(3) - (6)].integer) > MAX_FONTPOSITION)
				{
					yyerror("fontposition out of range");
					YYABORT;
				}
				Symbol = &yyFont->Symbol[(yyvsp[(3) - (6)].integer)];
				if (Symbol->Valid)
				{
					yyerror("symbol ID used twice");
					YYABORT;
				}
				Symbol->Valid = true;
				Symbol->Delta = OU ((yyvsp[(4) - (6)].measure));
			}
    break;

  case 174:
/* Line 1787 of yacc.c  */
#line 1790 "parse_y.y"
    {
				CreateNewLineInSymbol(Symbol, OU ((yyvsp[(3) - (8)].measure)), OU ((yyvsp[(4) - (8)].measure)), OU ((yyvsp[(5) - (8)].measure)), OU ((yyvsp[(6) - (8)].measure)), OU ((yyvsp[(7) - (8)].measure)));
			}
    break;

  case 175:
/* Line 1787 of yacc.c  */
#line 1797 "parse_y.y"
    {
				CreateNewLineInSymbol(Symbol, NU ((yyvsp[(3) - (8)].measure)), NU ((yyvsp[(4) - (8)].measure)), NU ((yyvsp[(5) - (8)].measure)), NU ((yyvsp[(6) - (8)].measure)), NU ((yyvsp[(7) - (8)].measure)));
			}
    break;

  case 183:
/* Line 1787 of yacc.c  */
#line 1851 "parse_y.y"
    {
				Menu = CreateNewNet(&yyPCB->NetlistLib, (yyvsp[(3) - (6)].string), (yyvsp[(4) - (6)].string));
				free ((yyvsp[(3) - (6)].string));
				free ((yyvsp[(4) - (6)].string));
			}
    break;

  case 189:
/* Line 1787 of yacc.c  */
#line 1886 "parse_y.y"
    {
				CreateNewConnection(Menu, (yyvsp[(3) - (4)].string));
				free ((yyvsp[(3) - (4)].string));
			}
    break;

  case 190:
/* Line 1787 of yacc.c  */
#line 1916 "parse_y.y"
    {
			  CreateNewAttribute (attr_list, (yyvsp[(3) - (5)].string), (yyvsp[(4) - (5)].string) ? (yyvsp[(4) - (5)].string) : (char *)"");
				free ((yyvsp[(3) - (5)].string));
				free ((yyvsp[(4) - (5)].string));
			}
    break;

  case 191:
/* Line 1787 of yacc.c  */
#line 1923 "parse_y.y"
    { (yyval.string) = (yyvsp[(1) - (1)].string); }
    break;

  case 192:
/* Line 1787 of yacc.c  */
#line 1924 "parse_y.y"
    { (yyval.string) = 0; }
    break;

  case 193:
/* Line 1787 of yacc.c  */
#line 1928 "parse_y.y"
    { (yyval.number) = (yyvsp[(1) - (1)].number); }
    break;

  case 194:
/* Line 1787 of yacc.c  */
#line 1929 "parse_y.y"
    { (yyval.number) = (yyvsp[(1) - (1)].integer); }
    break;

  case 195:
/* Line 1787 of yacc.c  */
#line 1934 "parse_y.y"
    { do_measure(&(yyval.measure), (yyvsp[(1) - (1)].number), MIL_TO_COORD ((yyvsp[(1) - (1)].number)) / 100.0, 0); }
    break;

  case 196:
/* Line 1787 of yacc.c  */
#line 1935 "parse_y.y"
    { M ((yyval.measure), (yyvsp[(1) - (2)].number), MIL_TO_COORD ((yyvsp[(1) - (2)].number)) / 100000.0); }
    break;

  case 197:
/* Line 1787 of yacc.c  */
#line 1936 "parse_y.y"
    { M ((yyval.measure), (yyvsp[(1) - (2)].number), MIL_TO_COORD ((yyvsp[(1) - (2)].number)) / 100.0); }
    break;

  case 198:
/* Line 1787 of yacc.c  */
#line 1937 "parse_y.y"
    { M ((yyval.measure), (yyvsp[(1) - (2)].number), MIL_TO_COORD ((yyvsp[(1) - (2)].number))); }
    break;

  case 199:
/* Line 1787 of yacc.c  */
#line 1938 "parse_y.y"
    { M ((yyval.measure), (yyvsp[(1) - (2)].number), INCH_TO_COORD ((yyvsp[(1) - (2)].number))); }
    break;

  case 200:
/* Line 1787 of yacc.c  */
#line 1939 "parse_y.y"
    { M ((yyval.measure), (yyvsp[(1) - (2)].number), MM_TO_COORD ((yyvsp[(1) - (2)].number)) / 1000000.0); }
    break;

  case 201:
/* Line 1787 of yacc.c  */
#line 1940 "parse_y.y"
    { M ((yyval.measure), (yyvsp[(1) - (2)].number), MM_TO_COORD ((yyvsp[(1) - (2)].number)) / 1000.0); }
    break;

  case 202:
/* Line 1787 of yacc.c  */
#line 1941 "parse_y.y"
    { M ((yyval.measure), (yyvsp[(1) - (2)].number), MM_TO_COORD ((yyvsp[(1) - (2)].number))); }
    break;

  case 203:
/* Line 1787 of yacc.c  */
#line 1942 "parse_y.y"
    { M ((yyval.measure), (yyvsp[(1) - (2)].number), MM_TO_COORD ((yyvsp[(1) - (2)].number)) * 1000.0); }
    break;

  case 204:
/* Line 1787 of yacc.c  */
#line 1943 "parse_y.y"
    { M ((yyval.measure), (yyvsp[(1) - (2)].number), MM_TO_COORD ((yyvsp[(1) - (2)].number)) * 1000000.0); }
    break;


/* Line 1787 of yacc.c  */
#line 3141 "parse_y.tab.c"
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

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
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

  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

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
  /* Do not reclaim the symbols of the rule which action triggered
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
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


/* Line 2050 of yacc.c  */
#line 1946 "parse_y.y"


/* ---------------------------------------------------------------------------
 * error routine called by parser library
 */
int yyerror(const char * s)
{
	Message("ERROR parsing file '%s'\n"
		"    line:        %i\n"
		"    description: '%s'\n",
		yyfilename, yylineno, s);
	return(0);
}

int yywrap()
{
  return 1;
}

static int
check_file_version (int ver)
{
  if ( ver > PCB_FILE_VERSION ) {
    Message ("ERROR:  The file you are attempting to load is in a format\n"
	     "which is too new for this version of pcb.  To load this file\n"
	     "you need a version of pcb which is >= %d.  If you are\n"
	     "using a version built from git source, the source date\n"
	     "must be >= %d.  This copy of pcb can only read files\n"
	     "up to file version %d.\n", ver, ver, PCB_FILE_VERSION);
    return 1;
  }
  
  return 0;
}

static void
do_measure (PLMeasure *m, Coord i, double d, int u)
{
  m->ival = i;
  m->bval = round (d);
  m->dval = d;
  m->has_units = u;
}

static int
integer_value (PLMeasure m)
{
  if (m.has_units)
    yyerror("units ignored here");
  return m.ival;
}

static Coord
old_units (PLMeasure m)
{
  if (m.has_units)
    return m.bval;
  return round (MIL_TO_COORD (m.ival));
}

static Coord
new_units (PLMeasure m)
{
  if (m.has_units)
    return m.bval;
  return round (MIL_TO_COORD (m.ival) / 100.0);
}
