/* A Bison parser, made by GNU Bison 3.0.2.  */

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
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 11 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:339  */

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

#include "config.h"
#include "conf_core.h"

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
#include "rats_patch.h"

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
extern	int	yyLoadSettings;

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


#line 164 "parse_y.tab.c" /* yacc.c:339  */

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
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "parse_y.tab.h".  */
#ifndef YY_YY_PARSE_Y_TAB_H_INCLUDED
# define YY_YY_PARSE_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
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
    T_NETLISTPATCH = 289,
    T_ADD_CONN = 290,
    T_DEL_CONN = 291,
    T_CHANGE_ATTRIB = 292,
    T_AREA = 293,
    T_THERMAL = 294,
    T_DRC = 295,
    T_ATTRIBUTE = 296,
    T_UMIL = 297,
    T_CMIL = 298,
    T_MIL = 299,
    T_IN = 300,
    T_NM = 301,
    T_UM = 302,
    T_MM = 303,
    T_M = 304,
    T_KM = 305
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 112 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:355  */

	int		integer;
	double		number;
	char		*string;
	FlagType	flagtype;
	PLMeasure	measure;

#line 263 "parse_y.tab.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PARSE_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 278 "parse_y.tab.c" /* yacc.c:358  */

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
#define YYFINAL  10
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   608

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  55
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  112
/* YYNRULES -- Number of rules.  */
#define YYNRULES  214
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  643

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   305

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
      53,    54,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    51,     2,    52,     2,     2,     2,     2,     2,     2,
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
static const yytype_uint16 yyrline[] =
{
       0,   139,   139,   140,   141,   142,   166,   166,   224,   224,
     235,   235,   254,   255,   260,   260,   300,   302,   332,   338,
     344,   373,   374,   375,   378,   386,   401,   435,   441,   447,
     463,   465,   490,   492,   523,   525,   526,   527,   531,   541,
     552,   579,   583,   588,   616,   620,   664,   673,   682,   686,
     687,   691,   692,   696,   697,   697,   698,   699,   701,   701,
     708,   712,   713,   714,   715,   716,   752,   762,   773,   783,
     793,   829,   834,   866,   865,   893,   894,   898,   899,   903,
     904,   905,   906,   907,   908,   910,   915,   916,   917,   918,
     918,   919,   949,   958,   967,  1015,  1024,  1033,  1070,  1080,
    1098,  1148,  1147,  1186,  1188,  1193,  1192,  1199,  1201,  1206,
    1210,  1270,  1271,  1272,  1273,  1274,  1282,  1281,  1300,  1299,
    1318,  1317,  1338,  1336,  1360,  1358,  1439,  1440,  1444,  1445,
    1446,  1447,  1448,  1450,  1455,  1460,  1465,  1470,  1475,  1480,
    1480,  1484,  1485,  1489,  1490,  1491,  1492,  1494,  1500,  1507,
    1512,  1517,  1517,  1558,  1570,  1582,  1593,  1609,  1663,  1677,
    1690,  1701,  1712,  1713,  1717,  1718,  1740,  1742,  1758,  1777,
    1778,  1781,  1783,  1784,  1805,  1812,  1828,  1829,  1833,  1838,
    1839,  1843,  1844,  1868,  1867,  1877,  1878,  1882,  1883,  1902,
    1919,  1920,  1924,  1929,  1930,  1934,  1935,  1950,  1951,  1952,
    1979,  1987,  1988,  1992,  1993,  1998,  1999,  2000,  2001,  2002,
    2003,  2004,  2005,  2006,  2007
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
  "T_POLYGON", "T_POLYGON_HOLE", "T_NETLIST", "T_NET", "T_CONN",
  "T_NETLISTPATCH", "T_ADD_CONN", "T_DEL_CONN", "T_CHANGE_ATTRIB",
  "T_AREA", "T_THERMAL", "T_DRC", "T_ATTRIBUTE", "T_UMIL", "T_CMIL",
  "T_MIL", "T_IN", "T_NM", "T_UM", "T_MM", "T_M", "T_KM", "'['", "']'",
  "'('", "')'", "$accept", "parse", "parsepcb", "$@1", "$@2", "parsedata",
  "$@3", "pcbfont", "parsefont", "$@4", "pcbfileversion", "pcbname",
  "pcbgrid", "pcbgridold", "pcbgridnew", "pcbhigrid", "pcbcursor",
  "polyarea", "pcbthermal", "pcbdrc", "pcbdrc1", "pcbdrc2", "pcbdrc3",
  "pcbflags", "pcbgroups", "pcbstyles", "pcbdata", "pcbdefinitions",
  "pcbdefinition", "$@5", "$@6", "via", "via_hi_format", "via_2.0_format",
  "via_1.7_format", "via_newformat", "via_oldformat", "rats", "layer",
  "$@7", "layerdata", "layerdefinitions", "layerdefinition", "$@8",
  "line_hi_format", "line_1.7_format", "line_oldformat", "arc_hi_format",
  "arc_1.7_format", "arc_oldformat", "text_oldformat", "text_newformat",
  "text_hi_format", "polygon_format", "$@9", "polygonholes", "polygonhole",
  "$@10", "polygonpoints", "polygonpoint", "element", "element_oldformat",
  "$@11", "element_1.3.4_format", "$@12", "element_newformat", "$@13",
  "element_1.7_format", "$@14", "element_hi_format", "$@15",
  "elementdefinitions", "elementdefinition", "$@16", "relementdefs",
  "relementdef", "$@17", "pin_hi_format", "pin_1.7_format",
  "pin_1.6.3_format", "pin_newformat", "pin_oldformat", "pad_hi_format",
  "pad_1.7_format", "pad_newformat", "pad", "flags", "symbols", "symbol",
  "symbolhead", "symbolid", "symboldata", "symboldefinition",
  "hiressymbol", "pcbnetlist", "pcbnetdef", "nets", "netdefs", "net",
  "$@18", "connections", "conndefs", "conn", "pcbnetlistpatch",
  "pcbnetpatchdef", "netpatches", "netpatchdefs", "netpatch", "attribute",
  "opt_string", "number", "measure", YY_NULLPTR
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
     305,    91,    93,    40,    41
};
# endif

#define YYPACT_NINF -449

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-449)))

#define YYTABLE_NINF -90

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     151,  -449,    39,  -449,     4,    30,  -449,    23,  -449,    56,
    -449,    32,    97,   -31,  -449,  -449,  -449,  -449,  -449,  -449,
    -449,    73,    -5,    26,  -449,   169,  -449,    88,    30,  -449,
    -449,  -449,  -449,  -449,  -449,  -449,  -449,    48,    56,  -449,
    -449,   129,    55,   116,    49,   113,   140,    63,    63,    63,
      63,  -449,    95,  -449,  -449,    84,    84,  -449,   -16,   105,
     143,   157,    92,   147,  -449,  -449,  -449,  -449,  -449,   158,
     161,   168,   176,  -449,  -449,   305,    63,    63,    63,    63,
     187,  -449,  -449,    63,    63,   134,  -449,  -449,  -449,  -449,
      63,     6,    63,    63,   160,   156,   191,   192,    63,   210,
    -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,    63,
      63,   224,   229,   233,   196,   197,    63,    63,    63,  -449,
      63,    63,    63,    63,    63,   218,   257,   272,   185,    63,
    -449,   236,    63,   221,    63,    63,   245,   255,   270,    63,
      63,   274,   287,    63,    63,    63,    63,    63,   283,   302,
      63,    63,    63,   353,   312,    63,   362,   239,    63,    63,
    -449,  -449,  -449,    63,    63,  -449,  -449,   378,     0,    63,
      63,   334,    63,   341,   372,  -449,  -449,  -449,    63,    63,
      63,   342,  -449,    63,   343,   394,   251,   397,   398,    63,
      63,   351,   350,  -449,   354,   355,  -449,   356,    63,   357,
     380,    63,    63,    63,   358,   260,   399,  -449,   359,   408,
     409,    49,   410,    63,    63,  -449,  -449,  -449,  -449,  -449,
      63,   228,   363,   389,    63,    63,   414,  -449,   205,   226,
     366,   253,   367,   368,   260,  -449,    88,  -449,  -449,  -449,
    -449,  -449,  -449,  -449,  -449,  -449,  -449,    49,  -449,   369,
     417,   373,   370,   376,   375,    63,   379,   381,   424,   256,
     412,    63,    90,   384,    33,    63,    63,    63,    63,    63,
      63,    63,    49,  -449,  -449,  -449,   385,  -449,   386,  -449,
    -449,  -449,  -449,    11,  -449,  -449,   387,   433,   436,   195,
    -449,    63,   390,    63,   393,   276,   403,   404,   279,   280,
     102,  -449,    88,  -449,  -449,  -449,  -449,  -449,    63,    63,
      63,    63,    63,    63,    63,   405,  -449,  -449,  -449,    13,
    -449,   391,   406,   416,    49,   411,   454,  -449,    63,    63,
      63,    63,    63,    63,    63,    63,  -449,  -449,  -449,    63,
      63,    63,    63,    63,    63,    63,   415,  -449,    63,  -449,
    -449,   418,   427,  -449,   413,  -449,   419,    33,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,   264,  -449,   420,   421,   423,  -449,  -449,   425,
      33,   426,   121,    63,    63,    63,    63,    63,    63,   422,
     437,    63,    63,    63,    63,   458,   457,   465,   464,   320,
    -449,   428,   438,  -449,   183,  -449,  -449,    63,    63,   299,
      63,    63,    63,  -449,  -449,    63,    63,    63,    63,   439,
      49,   440,   473,    63,    63,  -449,   320,   448,   442,   165,
    -449,   165,    63,    63,   486,   490,    63,    63,    63,    49,
       2,    63,    63,  -449,   445,  -449,   444,    63,    63,   -17,
    -449,   446,   447,   448,  -449,   -10,   321,   326,   327,   330,
     222,  -449,    88,  -449,  -449,  -449,  -449,   234,   450,   449,
     459,   352,   494,    63,    63,   460,   461,  -449,    63,   110,
    -449,  -449,   462,   463,   466,  -449,  -449,   510,  -449,  -449,
     467,   468,   469,   478,   -10,  -449,    63,    63,    63,    63,
      63,    63,    63,    63,  -449,  -449,  -449,  -449,  -449,  -449,
    -449,   479,   514,   383,    63,    63,  -449,  -449,    49,   480,
     519,  -449,  -449,  -449,   529,   530,   531,   538,  -449,  -449,
      63,    63,    63,    63,    63,    63,    63,    63,  -449,   493,
     495,   544,   502,   503,   506,  -449,   505,   320,   507,   554,
     556,   557,    63,    63,    63,    63,    63,    63,    63,    63,
    -449,  -449,   515,  -449,  -449,  -449,  -449,   516,   518,   520,
     521,   562,    63,    63,    63,    63,    63,    63,    63,    63,
    -449,  -449,  -449,  -449,  -449,   522,    63,    63,    63,    63,
      63,    63,    63,    63,   539,  -449,   525,   524,    63,    63,
      63,    63,    63,    63,   526,   527,   539,  -449,  -449,  -449,
     567,   574,    63,    63,    63,    63,   576,  -449,  -449,   577,
     578,   579,   580,   535,   534,   536,    49,   585,   586,   587,
    -449,  -449,  -449,   542,   537,    49,   591,  -449,  -449,   545,
     546,  -449,  -449
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
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
       0,     0,     0,   203,   204,   205,     0,     0,     0,     0,
       0,   169,   170,     0,     0,     0,   166,   172,   173,    17,
       0,     0,     0,     0,     0,    30,     0,     0,     0,   202,
     206,   207,   208,   209,   210,   211,   212,   213,   214,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    18,
       0,     0,     0,     0,     0,     0,    32,     0,     0,     0,
     201,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    34,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     200,   167,   168,     0,     0,    20,    19,     0,     0,     0,
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
      12,     0,   204,     0,     0,     0,     0,     0,     0,     0,
     139,   126,     0,   128,   129,   130,   131,   132,     0,     0,
       0,     0,     0,     0,     0,     0,    66,    67,    38,     0,
      44,     0,     0,   177,     0,     0,     0,   118,     0,     0,
       0,     0,     0,     0,     0,     0,   117,   127,   140,     0,
       0,     0,     0,     0,     0,     0,     0,    39,     0,    47,
      46,     0,   191,   176,     0,   120,     0,   139,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   101,     0,     0,     0,     7,   190,     0,
     139,     0,   139,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
      40,     0,     0,   124,   139,   122,   119,     0,     0,     0,
       0,     0,     0,   137,   138,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   103,   107,   180,     0,   151,
     121,   151,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    85,     0,    98,     0,     0,     0,     0,
     108,     0,     0,   179,   181,   194,     0,     0,     0,     0,
     151,   141,     0,   144,   143,   146,   145,   151,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    94,     0,     0,
     100,    99,     0,     0,     0,   102,   104,     0,   178,   182,
       0,     0,     0,     0,   193,   195,     0,     0,     0,     0,
       0,     0,     0,     0,   125,   142,   152,   123,   133,   134,
     157,     0,     0,     0,     0,     0,    92,    93,     0,   204,
       0,   110,   109,   105,     0,     0,     0,     0,   192,   196,
       0,     0,     0,     0,     0,     0,     0,     0,   156,     0,
       0,     0,     0,     0,     0,    97,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     155,   161,     0,   135,   136,    95,    96,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     160,   106,   183,   197,   198,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   186,   199,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   185,   187,   147,   148,
       0,     0,     0,     0,     0,     0,     0,   184,   188,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     149,   150,   189,     0,     0,     0,     0,   153,   154,     0,
       0,   158,   159
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,   336,  -449,
    -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,
    -449,  -449,  -449,  -449,  -449,  -449,   309,  -449,   581,  -449,
    -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,
    -449,  -449,   365,  -449,  -449,  -449,  -449,  -449,  -449,  -449,
    -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,  -423,  -449,
     573,  -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,
    -449,  -339,  -292,  -449,   171,  -448,  -449,  -449,  -449,  -449,
    -449,  -449,  -449,  -449,  -449,  -449,  -190,  -449,   565,  -449,
     548,  -449,  -449,  -449,  -449,  -449,  -449,  -449,   152,  -449,
    -449,  -449,     1,  -449,  -449,  -449,  -449,   114,  -231,  -449,
     -47,   -48
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     4,     5,     6,     7,   289,     8,     9,
      12,    43,    63,    64,    65,    66,    95,   126,   149,   174,
     175,   176,   177,   200,   223,   260,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,   205,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   399,   449,   486,   547,   425,   426,
      14,    15,   264,    16,   357,    17,   380,    18,   431,    19,
     429,   300,   301,   302,   460,   461,   462,   463,   464,   303,
     304,   305,   465,   466,   306,   307,    69,    38,    39,    40,
      83,    58,    87,    88,   352,   353,   452,   453,   454,   594,
     605,   606,   607,   377,   378,   493,   494,   495,    53,   131,
      75,    76
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      77,    78,    79,   450,   192,   275,   476,    85,   337,    73,
      74,    11,   505,   484,    73,    74,    73,    74,   382,   505,
      44,   251,    45,   -50,    20,   490,   491,   492,   109,   110,
     111,   112,    21,    22,    23,   114,   115,   485,    86,    10,
     -58,   404,   118,   120,   121,   122,    47,    13,    48,   295,
     129,   296,   297,    67,   193,    68,   477,   276,   298,   299,
     119,   132,   133,   318,   -54,   347,    73,    74,   139,   140,
     141,   338,   142,   143,   144,   145,   146,    49,    37,    50,
     152,   153,   315,    41,   155,   157,   158,   159,    81,    82,
     337,   163,   164,    73,   292,   167,   168,   169,   170,    55,
     171,    56,   178,   179,   180,    42,    60,   183,    61,   186,
     187,   188,   337,    73,   519,   189,   190,    70,   295,    71,
     296,   297,   194,   195,   567,   197,    46,   298,   299,    52,
     201,   202,   203,    59,   354,   206,    62,   295,   210,   296,
     297,   213,   214,    92,    72,    93,   298,   299,    80,    90,
     220,   -10,     1,   224,   225,   226,   336,    89,    -6,    -6,
     -10,   -10,   -10,    91,    96,   253,   254,    97,    -8,   -49,
      20,    94,   255,   -14,    98,   406,   261,   262,    21,    22,
      23,   456,    99,   457,   458,   116,   -58,   117,    73,    74,
     459,   151,   -10,   113,   125,   -50,    20,   127,   128,   295,
     -49,   296,   297,   -49,    21,    22,    23,   283,   298,   299,
     -54,   123,   -58,   124,   291,   293,   130,   308,   309,   310,
     311,   312,   313,   314,    73,    74,   -50,   156,   134,   -50,
     444,   506,   256,   135,   257,   319,   -54,   430,   456,   136,
     457,   458,    73,    74,   324,   185,   326,   459,   137,   475,
     456,   138,   457,   458,    73,    74,   265,   209,   266,   459,
     339,   340,   341,   342,   343,   344,   345,    73,    74,   147,
     397,   348,   228,   229,   230,   231,   504,   267,   150,   268,
     358,   359,   360,   361,   362,   363,   364,   365,   507,   232,
     154,   366,   367,   368,   369,   370,   148,   371,   372,   160,
     374,   -89,    73,    74,   270,   434,   271,   287,   161,   288,
     383,   384,   385,   386,   387,   388,   389,   390,   391,   392,
     393,   394,   395,   162,   396,   398,   165,   328,   544,   329,
     332,   334,   333,   335,   172,   407,   408,   409,   410,   411,
     412,   166,   173,   415,   416,   417,   418,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   511,   181,   512,   432,
     433,   435,   436,   437,   438,   182,   184,   439,   440,   441,
     442,   423,   496,   424,   497,   447,   448,   498,   500,   499,
     501,   502,   191,   503,   468,   469,   196,   540,   472,   541,
     473,   474,   198,   199,   478,   479,   204,   207,   208,   482,
     483,   211,   212,   215,   216,   247,   217,   222,   219,   218,
     221,   227,   249,   248,   252,   250,   258,   259,   263,   269,
     272,   278,   273,   277,   280,   279,   514,   515,   281,   282,
     286,   518,   520,   284,   -14,   285,   633,   316,   294,   321,
     317,   320,   322,   349,   325,   639,   327,   351,   530,   531,
     532,   533,   534,   535,   536,   537,   330,   331,   356,   346,
     350,   376,   419,   420,   355,   379,   542,   543,   373,   421,
     422,   375,   400,   381,   413,   401,   402,   446,   403,   405,
     451,   427,   552,   553,   554,   555,   556,   557,   558,   559,
     470,   414,   428,   443,   445,   455,   471,   480,   481,   487,
     513,   488,   508,   509,   572,   573,   574,   575,   576,   577,
     578,   579,   516,   510,   521,   517,   524,   522,   539,   523,
     525,   526,   527,   546,   586,   587,   588,   589,   590,   591,
     592,   593,   528,   538,   545,   548,   549,   550,   596,   597,
     598,   599,   600,   601,   551,   602,   603,   560,   562,   561,
     610,   611,   612,   613,   563,   614,   615,   564,   565,   566,
     569,   568,   570,   571,   621,   622,   623,   624,   585,   580,
     581,   582,   604,   619,   583,   584,   595,   608,   609,   616,
     620,   617,   625,   626,   627,   628,   629,   630,   631,   634,
     632,   638,   635,   636,   637,   640,   290,   641,   323,   274,
     642,    54,   467,    57,    84,   489,    51,   618,   529
};

static const yytype_uint16 yycheck[] =
{
      48,    49,    50,   426,     4,   236,     4,    23,   300,     3,
       4,     7,   460,    30,     3,     4,     3,     4,   357,   467,
      51,   211,    53,     0,     1,    35,    36,    37,    76,    77,
      78,    79,     9,    10,    11,    83,    84,    54,    54,     0,
      17,   380,    90,    91,    92,    93,    51,    17,    53,    16,
      98,    18,    19,     4,    54,     6,    54,   247,    25,    26,
      54,   109,   110,    52,    41,    52,     3,     4,   116,   117,
     118,   302,   120,   121,   122,   123,   124,    51,    22,    53,
     128,   129,   272,    51,   132,   133,   134,   135,     4,     5,
     382,   139,   140,     3,     4,   143,   144,   145,   146,    51,
     147,    53,   150,   151,   152,     8,    51,   155,    53,   157,
     158,   159,   404,     3,     4,   163,   164,     4,    16,     6,
      18,    19,   169,   170,   547,   172,    53,    25,    26,    41,
     178,   179,   180,     4,   324,   183,    20,    16,   186,    18,
      19,   189,   190,    51,     4,    53,    25,    26,    53,     6,
     198,     0,     1,   201,   202,   203,    54,    52,     7,     8,
       9,    10,    11,     6,     6,   213,   214,     6,    17,     0,
       1,    24,   220,    22,     6,    54,   224,   225,     9,    10,
      11,    16,     6,    18,    19,    51,    17,    53,     3,     4,
      25,     6,    41,     6,    38,     0,     1,     6,     6,    16,
      31,    18,    19,    34,     9,    10,    11,   255,    25,    26,
      41,    51,    17,    53,   261,   262,     6,   265,   266,   267,
     268,   269,   270,   271,     3,     4,    31,     6,     4,    34,
     420,   462,     4,     4,     6,   283,    41,    54,    16,     6,
      18,    19,     3,     4,   291,     6,   293,    25,    52,   439,
      16,    54,    18,    19,     3,     4,    51,     6,    53,    25,
     308,   309,   310,   311,   312,   313,   314,     3,     4,    51,
       6,   319,    12,    13,    14,    15,    54,    51,     6,    53,
     328,   329,   330,   331,   332,   333,   334,   335,    54,    29,
      54,   339,   340,   341,   342,   343,    39,   344,   345,    54,
     348,    41,     3,     4,    51,     6,    53,    51,    53,    53,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,    53,   371,   372,    52,    51,   518,    53,
      51,    51,    53,    53,    51,   383,   384,   385,   386,   387,
     388,    54,    40,   391,   392,   393,   394,    42,    43,    44,
      45,    46,    47,    48,    49,    50,     4,     4,     6,   407,
     408,   409,   410,   411,   412,    53,     4,   415,   416,   417,
     418,    51,    51,    53,    53,   423,   424,    51,    51,    53,
      53,    51,     4,    53,   432,   433,    52,     4,   436,     6,
     437,   438,    51,    21,   441,   442,    54,    54,     4,   447,
     448,     4,     4,    52,    54,     6,    52,    27,    52,    54,
      53,    53,     4,    54,     4,     6,    53,    28,     4,    53,
      53,     4,    54,    54,    54,    52,   473,   474,    52,    54,
       6,   478,   479,    54,    22,    54,   626,    52,    54,     6,
      54,    54,     6,    52,    54,   635,    53,    31,   496,   497,
     498,   499,   500,   501,   502,   503,    53,    53,     4,    54,
      54,    34,     4,     6,    53,    52,   514,   515,    53,     4,
       6,    53,    52,    54,    52,    54,    53,     4,    53,    53,
      32,    53,   530,   531,   532,   533,   534,   535,   536,   537,
       4,    54,    54,    54,    54,    53,     6,    52,    54,    53,
       6,    54,    52,    54,   552,   553,   554,   555,   556,   557,
     558,   559,    52,    54,    52,    54,     6,    54,     4,    53,
      53,    53,    53,     4,   572,   573,   574,   575,   576,   577,
     578,   579,    54,    54,    54,     6,     6,     6,   586,   587,
     588,   589,   590,   591,     6,   592,   593,    54,     4,    54,
     598,   599,   600,   601,    52,   602,   603,    54,    52,    54,
       6,    54,     6,     6,   612,   613,   614,   615,     6,    54,
      54,    53,    33,     6,    54,    54,    54,    52,    54,    53,
       6,    54,     6,     6,     6,     6,     6,    52,    54,     4,
      54,    54,     6,     6,    52,     4,   260,    52,   289,   234,
      54,    28,   431,    38,    56,   453,    25,   606,   494
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    56,    57,    58,    59,    60,    61,    63,    64,
       0,     7,    65,    17,   115,   116,   118,   120,   122,   124,
       1,     9,    10,    11,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    22,   142,   143,
     144,    51,     8,    66,    51,    53,    53,    51,    53,    51,
      53,    83,    41,   163,   115,    51,    53,   143,   146,     4,
      51,    53,    20,    67,    68,    69,    70,     4,     6,   141,
       4,     6,     4,     3,     4,   165,   166,   166,   166,   166,
      53,     4,     5,   145,   145,    23,    54,   147,   148,    52,
       6,     6,    51,    53,    24,    71,     6,     6,     6,     6,
      42,    43,    44,    45,    46,    47,    48,    49,    50,   166,
     166,   166,   166,     6,   166,   166,    51,    53,   166,    54,
     166,   166,   166,    51,    53,    38,    72,     6,     6,   166,
       6,   164,   166,   166,     4,     4,     6,    52,    54,   166,
     166,   166,   166,   166,   166,   166,   166,    51,    39,    73,
       6,     6,   166,   166,    54,   166,     6,   166,   166,   166,
      54,    53,    53,   166,   166,    52,    54,   166,   166,   166,
     166,   165,    51,    40,    74,    75,    76,    77,   166,   166,
     166,     4,    53,   166,     4,     6,   166,   166,   166,   166,
     166,     4,     4,    54,   165,   165,    52,   165,    51,    21,
      78,   166,   166,   166,    54,    94,   166,    54,     4,     6,
     166,     4,     4,   166,   166,    52,    54,    52,    54,    52,
     166,    53,    27,    79,   166,   166,   166,    53,    12,    13,
      14,    15,    29,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,     6,    54,     4,
       6,   141,     4,   166,   166,   166,     4,     6,    53,    28,
      80,   166,   166,     4,   117,    51,    53,    51,    53,    53,
      51,    53,    53,    54,    97,   163,   141,    54,     4,    52,
      54,    52,    54,   166,    54,    54,     6,    51,    53,    62,
      63,   165,     4,   165,    54,    16,    18,    19,    25,    26,
     126,   127,   128,   134,   135,   136,   139,   140,   166,   166,
     166,   166,   166,   166,   166,   141,    52,    54,    52,   166,
      54,     6,     6,    81,   165,    54,   165,    53,    51,    53,
      53,    53,    51,    53,    51,    53,    54,   127,   163,   166,
     166,   166,   166,   166,   166,   166,    54,    52,   166,    52,
      54,    31,   149,   150,   141,    53,     4,   119,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   165,   165,    53,   166,    53,    34,   158,   159,    52,
     121,    54,   126,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   165,     6,   165,   109,
      52,    54,    53,    53,   126,    53,    54,   166,   166,   166,
     166,   166,   166,    52,    54,   166,   166,   166,   166,     4,
       6,     4,     6,    51,    53,   113,   114,    53,    54,   125,
      54,   123,   166,   166,     6,   166,   166,   166,   166,   166,
     166,   166,   166,    54,   141,    54,     4,   166,   166,   110,
     113,    32,   151,   152,   153,    53,    16,    18,    19,    25,
     129,   130,   131,   132,   133,   137,   138,   129,   166,   166,
       4,     6,   166,   165,   165,   141,     4,    54,   165,   165,
      52,    54,   166,   166,    30,    54,   111,    53,    54,   153,
      35,    36,    37,   160,   161,   162,    51,    53,    51,    53,
      51,    53,    51,    53,    54,   130,   163,    54,    52,    54,
      54,     4,     6,     6,   165,   165,    52,    54,   165,     4,
     165,    52,    54,    53,     6,    53,    53,    53,    54,   162,
     166,   166,   166,   166,   166,   166,   166,   166,    54,     4,
       4,     6,   166,   166,   141,    54,     4,   112,     6,     6,
       6,     6,   166,   166,   166,   166,   166,   166,   166,   166,
      54,    54,     4,    52,    54,    52,    54,   113,    54,     6,
       6,     6,   166,   166,   166,   166,   166,   166,   166,   166,
      54,    54,    53,    54,    54,     6,   166,   166,   166,   166,
     166,   166,   166,   166,   154,    54,   166,   166,   166,   166,
     166,   166,   165,   165,    33,   155,   156,   157,    52,    54,
     166,   166,   166,   166,   165,   165,    53,    54,   157,     6,
       6,   166,   166,   166,   166,     6,     6,     6,     6,     6,
      52,    54,    54,   141,     4,     6,     6,    52,    54,   141,
       4,    52,    54
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    55,    56,    56,    56,    56,    58,    57,    59,    57,
      61,    60,    62,    62,    64,    63,    65,    65,    66,    66,
      66,    67,    67,    67,    68,    69,    70,    71,    71,    71,
      72,    72,    73,    73,    74,    74,    74,    74,    75,    76,
      77,    78,    78,    78,    79,    79,    80,    80,    80,    81,
      81,    82,    82,    83,    84,    83,    83,    83,    85,    83,
      83,    86,    86,    86,    86,    86,    87,    88,    89,    90,
      91,    92,    92,    94,    93,    95,    95,    96,    96,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,    98,
      97,    97,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   109,   108,   110,   110,   112,   111,   113,   113,   114,
     114,   115,   115,   115,   115,   115,   117,   116,   119,   118,
     121,   120,   123,   122,   125,   124,   126,   126,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   128,
     127,   129,   129,   130,   130,   130,   130,   130,   130,   130,
     130,   131,   130,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   141,   142,   142,   143,   144,   144,   145,
     145,   146,   146,   146,   147,   148,   149,   149,   150,   151,
     151,   152,   152,   154,   153,   155,   155,   156,   156,   157,
     158,   158,   159,   160,   160,   161,   161,   162,   162,   162,
     163,   164,   164,   165,   165,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     0,    15,     0,     2,
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
       1,     0,     6,     1,     0,     1,     2,     5,     5,     6,
       5,     1,     0,     1,     1,     1,     2,     2,     2,     2,
       2,     2,     2,     2,     2
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
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
        case 5:
#line 142 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { YYABORT; }
#line 1780 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 166 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
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
				yyPCB->NetlistPatches = yyPCB->NetlistPatchLast = NULL;
				layer_group_string = NULL;
			}
#line 1805 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 200 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
			  PCBTypePtr pcb_save = PCB;

			  if (yyLoadSettings && (layer_group_string == NULL))
					conf_set(CFR_DESIGN, "design/groups", -1, conf_core.design.groups, POL_OVERWRITE);
			  CreateNewPCBPost (yyPCB, 0);
			  if (ParseGroupString(layer_group_string, &yyPCB->LayerGroups, yyData->LayerN))
			    {
			      Message("illegal layer-group string\n");
			      YYABORT;
			    }
			/* initialize the polygon clipping now since
			 * we didn't know the layer grouping before.
			 */
			free(layer_group_string);
			PCB = yyPCB;
			ALLPOLYGON_LOOP (yyData);
			{
			  InitClip (yyData, layer, polygon);
			}
			ENDALL_LOOP;
			PCB = pcb_save;
			}
#line 1833 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 224 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { PreLoadElementPCB ();
		    layer_group_string = NULL; }
#line 1840 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 227 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { LayerFlag[0] = true;
		    LayerFlag[1] = true;
		    yyData->LayerN = 2;
		    PostLoadElementPCB ();
		  }
#line 1850 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 235 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
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
#line 1870 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 260 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
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
#line 1889 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 275 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyFont->Valid = true;
		  		SetFontInfo(yyFont);
			}
#line 1898 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 303 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
  if (check_file_version ((yyvsp[-1].integer)) != 0)
    {
      YYABORT;
    }
}
#line 1909 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 333 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->Name = (yyvsp[-1].string);
				yyPCB->MaxWidth = MAX_COORD;
				yyPCB->MaxHeight = MAX_COORD;
			}
#line 1919 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 339 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->Name = (yyvsp[-3].string);
				yyPCB->MaxWidth = OU ((yyvsp[-2].measure));
				yyPCB->MaxHeight = OU ((yyvsp[-1].measure));
			}
#line 1929 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 345 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->Name = (yyvsp[-3].string);
				yyPCB->MaxWidth = NU ((yyvsp[-2].measure));
				yyPCB->MaxHeight = NU ((yyvsp[-1].measure));
			}
#line 1939 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 379 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->Grid = OU ((yyvsp[-3].measure));
				yyPCB->GridOffsetX = OU ((yyvsp[-2].measure));
				yyPCB->GridOffsetY = OU ((yyvsp[-1].measure));
			}
#line 1949 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 387 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->Grid = OU ((yyvsp[-4].measure));
				yyPCB->GridOffsetX = OU ((yyvsp[-3].measure));
				yyPCB->GridOffsetY = OU ((yyvsp[-2].measure));
				if (yyLoadSettings) {
					if ((yyvsp[-1].integer))
						conf_set(CFR_DESIGN, "editor/draw_grid", -1, "true", POL_OVERWRITE);
					else
						conf_set(CFR_DESIGN, "editor/draw_grid", -1, "false", POL_OVERWRITE);
				}
			}
#line 1965 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 402 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->Grid = NU ((yyvsp[-4].measure));
				yyPCB->GridOffsetX = NU ((yyvsp[-3].measure));
				yyPCB->GridOffsetY = NU ((yyvsp[-2].measure));
				if (yyLoadSettings) {
					if ((yyvsp[-1].integer))
						conf_set(CFR_DESIGN, "editor/draw_grid", -1, "true", POL_OVERWRITE);
					else
						conf_set(CFR_DESIGN, "editor/draw_grid", -1, "false", POL_OVERWRITE);
				}
			}
#line 1981 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 436 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->CursorX = OU ((yyvsp[-3].measure));
				yyPCB->CursorY = OU ((yyvsp[-2].measure));
				yyPCB->Zoom = (yyvsp[-1].number)*2;
			}
#line 1991 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 442 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->CursorX = NU ((yyvsp[-3].measure));
				yyPCB->CursorY = NU ((yyvsp[-2].measure));
				yyPCB->Zoom = (yyvsp[-1].number);
			}
#line 2001 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 466 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				/* Read in cmil^2 for now; in future this should be a noop. */
				yyPCB->IsleArea = MIL_TO_COORD (MIL_TO_COORD ((yyvsp[-1].number)) / 100.0) / 100.0;
			}
#line 2010 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 493 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->ThermScale = (yyvsp[-1].number);
			}
#line 2018 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 532 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->Bloat = NU ((yyvsp[-3].measure));
				yyPCB->Shrink = NU ((yyvsp[-2].measure));
				yyPCB->minWid = NU ((yyvsp[-1].measure));
				yyPCB->minRing = NU ((yyvsp[-1].measure));
			}
#line 2029 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 542 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->Bloat = NU ((yyvsp[-4].measure));
				yyPCB->Shrink = NU ((yyvsp[-3].measure));
				yyPCB->minWid = NU ((yyvsp[-2].measure));
				yyPCB->minSlk = NU ((yyvsp[-1].measure));
				yyPCB->minRing = NU ((yyvsp[-2].measure));
			}
#line 2041 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 553 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->Bloat = NU ((yyvsp[-6].measure));
				yyPCB->Shrink = NU ((yyvsp[-5].measure));
				yyPCB->minWid = NU ((yyvsp[-4].measure));
				yyPCB->minSlk = NU ((yyvsp[-3].measure));
				yyPCB->minDrill = NU ((yyvsp[-2].measure));
				yyPCB->minRing = NU ((yyvsp[-1].measure));
			}
#line 2054 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 580 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyPCB->Flags = MakeFlags ((yyvsp[-1].integer) & PCB_FLAGS);
			}
#line 2062 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 584 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
			  yyPCB->Flags = string_to_pcbflags ((yyvsp[-1].string), yyerror);
			  free((yyvsp[-1].string));
			}
#line 2071 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 617 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
			  layer_group_string = (yyvsp[-1].string);
			}
#line 2079 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 665 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				if (ParseRouteString((yyvsp[-1].string), &yyPCB->RouteStyle[0], "mil"))
				{
					Message("illegal route-style string\n");
					YYABORT;
				}
				free((yyvsp[-1].string));
			}
#line 2092 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 674 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				if (ParseRouteString((yyvsp[-1].string), &yyPCB->RouteStyle[0], "cmil"))
				{
					Message("illegal route-style string\n");
					YYABORT;
				}
				free((yyvsp[-1].string));
			}
#line 2105 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 697 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { attr_list = & yyPCB->Attributes; }
#line 2111 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 701 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
					/* clear pointer to force memory allocation by 
					 * the appropriate subroutine
					 */
				yyElement = NULL;
			}
#line 2122 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 708 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { YYABORT; }
#line 2128 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 753 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewVia(yyData, NU ((yyvsp[-8].measure)), NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)),
				                     NU ((yyvsp[-3].measure)), (yyvsp[-2].string), (yyvsp[-1].flagtype));
				free ((yyvsp[-2].string));
			}
#line 2138 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 763 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewVia(yyData, OU ((yyvsp[-8].measure)), OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-2].string),
					OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2148 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 774 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewVia(yyData, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)),
					     OU ((yyvsp[-5].measure)) + OU((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-2].string), OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2158 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 784 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewVia(yyData, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), 2*GROUNDPLANEFRAME,
					OU((yyvsp[-4].measure)) + 2*MASKFRAME,  OU ((yyvsp[-3].measure)), (yyvsp[-2].string), OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2168 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 794 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				Coord	hole = (OU((yyvsp[-3].measure)) * DEFAULT_DRILLINGHOLE);

					/* make sure that there's enough copper left */
				if (OU((yyvsp[-3].measure)) - hole < MIN_PINORVIACOPPER && 
					OU((yyvsp[-3].measure)) > MIN_PINORVIACOPPER)
					hole = OU((yyvsp[-3].measure)) - MIN_PINORVIACOPPER;

				CreateNewVia(yyData, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), 2*GROUNDPLANEFRAME,
					OU((yyvsp[-3].measure)) + 2*MASKFRAME, hole, (yyvsp[-2].string), OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2185 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 830 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewRat(yyData, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-4].measure)), NU ((yyvsp[-3].measure)), (yyvsp[-5].integer), (yyvsp[-2].integer),
					conf_core.appearance.rat_thickness, (yyvsp[-1].flagtype));
			}
#line 2194 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 835 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewRat(yyData, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-5].integer), (yyvsp[-2].integer),
					conf_core.appearance.rat_thickness, OldFlags((yyvsp[-1].integer)));
			}
#line 2203 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 866 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				if ((yyvsp[-4].integer) <= 0 || (yyvsp[-4].integer) > MAX_LAYER + 2)
				{
					yyerror("Layernumber out of range");
					YYABORT;
				}
				if (LayerFlag[(yyvsp[-4].integer)-1])
				{
					yyerror("Layernumber used twice");
					YYABORT;
				}
				Layer = &yyData->Layer[(yyvsp[-4].integer)-1];

					/* memory for name is already allocated */
				if (Layer->Name != NULL)
					free(Layer->Name);
				Layer->Name = (yyvsp[-3].string);
				LayerFlag[(yyvsp[-4].integer)-1] = true;
				if (yyData->LayerN + 2 < (yyvsp[-4].integer))
				  yyData->LayerN = (yyvsp[-4].integer) - 2;
				if ((yyvsp[-2].string) != NULL)
					free((yyvsp[-2].string));
			}
#line 2231 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 911 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewPolygonFromRectangle(Layer,
					OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-5].measure)) + OU ((yyvsp[-3].measure)), OU ((yyvsp[-4].measure)) + OU ((yyvsp[-2].measure)), OldFlags((yyvsp[-1].integer)));
			}
#line 2240 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 918 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { attr_list = & Layer->Attributes; }
#line 2246 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 950 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewLineOnLayer(Layer, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)),
				                            NU ((yyvsp[-3].measure)), NU ((yyvsp[-2].measure)), (yyvsp[-1].flagtype));
			}
#line 2255 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 959 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewLineOnLayer(Layer, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)),
						     OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)), OldFlags((yyvsp[-1].integer)));
			}
#line 2264 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 968 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				/* eliminate old-style rat-lines */
			if ((IV ((yyvsp[-1].measure)) & RATFLAG) == 0)
				CreateNewLineOnLayer(Layer, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)),
					200*GROUNDPLANEFRAME, OldFlags(IV ((yyvsp[-1].measure))));
			}
#line 2275 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 1016 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
			  CreateNewArcOnLayer(Layer, NU ((yyvsp[-9].measure)), NU ((yyvsp[-8].measure)), NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), (yyvsp[-3].number), (yyvsp[-2].number),
			                             NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-1].flagtype));
			}
#line 2284 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 1025 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewArcOnLayer(Layer, OU ((yyvsp[-9].measure)), OU ((yyvsp[-8].measure)), OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), (yyvsp[-3].number), (yyvsp[-2].number),
						    OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OldFlags((yyvsp[-1].integer)));
			}
#line 2293 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 1034 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewArcOnLayer(Layer, OU ((yyvsp[-8].measure)), OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-6].measure)), IV ((yyvsp[-3].measure)), (yyvsp[-2].number),
					OU ((yyvsp[-4].measure)), 200*GROUNDPLANEFRAME, OldFlags((yyvsp[-1].integer)));
			}
#line 2302 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 1071 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
					/* use a default scale of 100% */
				CreateNewText(Layer,yyFont,OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].number), 100, (yyvsp[-2].string), OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2312 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 1081 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				if ((yyvsp[-1].integer) & ONSILKFLAG)
				{
					LayerTypePtr lay = &yyData->Layer[yyData->LayerN +
						(((yyvsp[-1].integer) & ONSOLDERFLAG) ? SOLDER_LAYER : COMPONENT_LAYER)];

					CreateNewText(lay ,yyFont, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), (yyvsp[-4].number), (yyvsp[-3].number), (yyvsp[-2].string),
						      OldFlags((yyvsp[-1].integer)));
				}
				else
					CreateNewText(Layer, yyFont, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), (yyvsp[-4].number), (yyvsp[-3].number), (yyvsp[-2].string),
						      OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2331 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 1099 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				/* FIXME: shouldn't know about .f */
				/* I don't think this matters because anything with hi_format
				 * will have the silk on its own layer in the file rather
				 * than using the ONSILKFLAG and having it in a copper layer.
				 * Thus there is no need for anything besides the 'else'
				 * part of this code.
				 */
				if ((yyvsp[-1].flagtype).f & ONSILKFLAG)
				{
					LayerTypePtr lay = &yyData->Layer[yyData->LayerN +
						(((yyvsp[-1].flagtype).f & ONSOLDERFLAG) ? SOLDER_LAYER : COMPONENT_LAYER)];

					CreateNewText(lay, yyFont, NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), (yyvsp[-4].number), (yyvsp[-3].number), (yyvsp[-2].string), (yyvsp[-1].flagtype));
				}
				else
					CreateNewText(Layer, yyFont, NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), (yyvsp[-4].number), (yyvsp[-3].number), (yyvsp[-2].string), (yyvsp[-1].flagtype));
				free ((yyvsp[-2].string));
			}
#line 2355 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 1148 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				Polygon = CreateNewPolygon(Layer, (yyvsp[-2].flagtype));
			}
#line 2363 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 1153 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
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
#line 2399 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 1193 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewHoleInPolygon (Polygon);
			}
#line 2407 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 1207 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewPointInPolygon(Polygon, OU ((yyvsp[-2].measure)), OU ((yyvsp[-1].measure)));
			}
#line 2415 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 1211 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewPointInPolygon(Polygon, NU ((yyvsp[-2].measure)), NU ((yyvsp[-1].measure)));
			}
#line 2423 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 1282 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyElement = CreateNewElement(yyData, yyElement, yyFont, NoFlags(),
					(yyvsp[-6].string), (yyvsp[-5].string), NULL, OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-2].integer), 100, NoFlags(), false);
				free ((yyvsp[-6].string));
				free ((yyvsp[-5].string));
				pin_num = 1;
			}
#line 2435 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 1290 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
#line 2443 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 1300 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyElement = CreateNewElement(yyData, yyElement, yyFont, OldFlags((yyvsp[-9].integer)),
					(yyvsp[-8].string), (yyvsp[-7].string), NULL, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), IV ((yyvsp[-4].measure)), IV ((yyvsp[-3].measure)), OldFlags((yyvsp[-2].integer)), false);
				free ((yyvsp[-8].string));
				free ((yyvsp[-7].string));
				pin_num = 1;
			}
#line 2455 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 1308 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
#line 2463 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 1318 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyElement = CreateNewElement(yyData, yyElement, yyFont, OldFlags((yyvsp[-10].integer)),
					(yyvsp[-9].string), (yyvsp[-8].string), (yyvsp[-7].string), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), IV ((yyvsp[-4].measure)), IV ((yyvsp[-3].measure)), OldFlags((yyvsp[-2].integer)), false);
				free ((yyvsp[-9].string));
				free ((yyvsp[-8].string));
				free ((yyvsp[-7].string));
				pin_num = 1;
			}
#line 2476 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 1327 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
#line 2484 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 1338 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyElement = CreateNewElement(yyData, yyElement, yyFont, OldFlags((yyvsp[-12].integer)),
					(yyvsp[-11].string), (yyvsp[-10].string), (yyvsp[-9].string), OU ((yyvsp[-8].measure)) + OU ((yyvsp[-6].measure)), OU ((yyvsp[-7].measure)) + OU ((yyvsp[-5].measure)),
					(yyvsp[-4].number), (yyvsp[-3].number), OldFlags((yyvsp[-2].integer)), false);
				yyElement->MarkX = OU ((yyvsp[-8].measure));
				yyElement->MarkY = OU ((yyvsp[-7].measure));
				free ((yyvsp[-11].string));
				free ((yyvsp[-10].string));
				free ((yyvsp[-9].string));
			}
#line 2499 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 1349 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
#line 2507 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 1360 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyElement = CreateNewElement(yyData, yyElement, yyFont, (yyvsp[-12].flagtype),
					(yyvsp[-11].string), (yyvsp[-10].string), (yyvsp[-9].string), NU ((yyvsp[-8].measure)) + NU ((yyvsp[-6].measure)), NU ((yyvsp[-7].measure)) + NU ((yyvsp[-5].measure)),
					(yyvsp[-4].number), (yyvsp[-3].number), (yyvsp[-2].flagtype), false);
				yyElement->MarkX = NU ((yyvsp[-8].measure));
				yyElement->MarkY = NU ((yyvsp[-7].measure));
				free ((yyvsp[-11].string));
				free ((yyvsp[-10].string));
				free ((yyvsp[-9].string));
			}
#line 2522 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 1371 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
#line 2530 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 1451 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewLineInElement(yyElement, NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), NU ((yyvsp[-3].measure)), NU ((yyvsp[-2].measure)), NU ((yyvsp[-1].measure)));
			}
#line 2538 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 134:
#line 1456 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewLineInElement(yyElement, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)), OU ((yyvsp[-1].measure)));
			}
#line 2546 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 135:
#line 1461 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewArcInElement(yyElement, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), NU ((yyvsp[-1].measure)));
			}
#line 2554 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 136:
#line 1466 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewArcInElement(yyElement, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), OU ((yyvsp[-1].measure)));
			}
#line 2562 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 137:
#line 1471 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyElement->MarkX = NU ((yyvsp[-2].measure));
				yyElement->MarkY = NU ((yyvsp[-1].measure));
			}
#line 2571 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 138:
#line 1476 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				yyElement->MarkX = OU ((yyvsp[-2].measure));
				yyElement->MarkY = OU ((yyvsp[-1].measure));
			}
#line 2580 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 139:
#line 1480 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { attr_list = & yyElement->Attributes; }
#line 2586 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 147:
#line 1495 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewLineInElement(yyElement, NU ((yyvsp[-5].measure)) + yyElement->MarkX,
					NU ((yyvsp[-4].measure)) + yyElement->MarkY, NU ((yyvsp[-3].measure)) + yyElement->MarkX,
					NU ((yyvsp[-2].measure)) + yyElement->MarkY, NU ((yyvsp[-1].measure)));
			}
#line 2596 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 148:
#line 1501 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewLineInElement(yyElement, OU ((yyvsp[-5].measure)) + yyElement->MarkX,
					OU ((yyvsp[-4].measure)) + yyElement->MarkY, OU ((yyvsp[-3].measure)) + yyElement->MarkX,
					OU ((yyvsp[-2].measure)) + yyElement->MarkY, OU ((yyvsp[-1].measure)));
			}
#line 2606 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 149:
#line 1508 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewArcInElement(yyElement, NU ((yyvsp[-7].measure)) + yyElement->MarkX,
					NU ((yyvsp[-6].measure)) + yyElement->MarkY, NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), NU ((yyvsp[-1].measure)));
			}
#line 2615 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 150:
#line 1513 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewArcInElement(yyElement, OU ((yyvsp[-7].measure)) + yyElement->MarkX,
					OU ((yyvsp[-6].measure)) + yyElement->MarkY, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), OU ((yyvsp[-1].measure)));
			}
#line 2624 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 151:
#line 1517 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { attr_list = & yyElement->Attributes; }
#line 2630 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 153:
#line 1559 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewPin(yyElement, NU ((yyvsp[-9].measure)) + yyElement->MarkX,
					NU ((yyvsp[-8].measure)) + yyElement->MarkY, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-3].string),
					(yyvsp[-2].string), (yyvsp[-1].flagtype));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2642 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 154:
#line 1571 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewPin(yyElement, OU ((yyvsp[-9].measure)) + yyElement->MarkX,
					OU ((yyvsp[-8].measure)) + yyElement->MarkY, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].string),
					(yyvsp[-2].string), OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2654 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 155:
#line 1583 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewPin(yyElement, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), 2*GROUNDPLANEFRAME,
					OU ((yyvsp[-5].measure)) + 2*MASKFRAME, OU ((yyvsp[-4].measure)), (yyvsp[-3].string), (yyvsp[-2].string), OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2665 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 156:
#line 1594 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				char	p_number[8];

				sprintf(p_number, "%d", pin_num++);
				CreateNewPin(yyElement, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), 2*GROUNDPLANEFRAME,
					OU ((yyvsp[-4].measure)) + 2*MASKFRAME, OU ((yyvsp[-3].measure)), (yyvsp[-2].string), p_number, OldFlags((yyvsp[-1].integer)));

				free ((yyvsp[-2].string));
			}
#line 2679 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 157:
#line 1610 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				Coord	hole = OU ((yyvsp[-3].measure)) * DEFAULT_DRILLINGHOLE;
				char	p_number[8];

					/* make sure that there's enough copper left */
				if (OU ((yyvsp[-3].measure)) - hole < MIN_PINORVIACOPPER && 
					OU ((yyvsp[-3].measure)) > MIN_PINORVIACOPPER)
					hole = OU ((yyvsp[-3].measure)) - MIN_PINORVIACOPPER;

				sprintf(p_number, "%d", pin_num++);
				CreateNewPin(yyElement, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), 2*GROUNDPLANEFRAME,
					OU ((yyvsp[-3].measure)) + 2*MASKFRAME, hole, (yyvsp[-2].string), p_number, OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2698 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 158:
#line 1664 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewPad(yyElement, NU ((yyvsp[-10].measure)) + yyElement->MarkX,
					NU ((yyvsp[-9].measure)) + yyElement->MarkY,
					NU ((yyvsp[-8].measure)) + yyElement->MarkX,
					NU ((yyvsp[-7].measure)) + yyElement->MarkY, NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)),
					(yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].flagtype));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2712 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 159:
#line 1678 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewPad(yyElement,OU ((yyvsp[-10].measure)) + yyElement->MarkX,
					OU ((yyvsp[-9].measure)) + yyElement->MarkY, OU ((yyvsp[-8].measure)) + yyElement->MarkX,
					OU ((yyvsp[-7].measure)) + yyElement->MarkY, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)),
					(yyvsp[-3].string), (yyvsp[-2].string), OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2725 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 160:
#line 1691 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewPad(yyElement,OU ((yyvsp[-8].measure)),OU ((yyvsp[-7].measure)),OU ((yyvsp[-6].measure)),OU ((yyvsp[-5].measure)),OU ((yyvsp[-4].measure)), 2*GROUNDPLANEFRAME,
					OU ((yyvsp[-4].measure)) + 2*MASKFRAME, (yyvsp[-3].string), (yyvsp[-2].string), OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2736 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 161:
#line 1702 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				char		p_number[8];

				sprintf(p_number, "%d", pin_num++);
				CreateNewPad(yyElement,OU ((yyvsp[-7].measure)),OU ((yyvsp[-6].measure)),OU ((yyvsp[-5].measure)),OU ((yyvsp[-4].measure)),OU ((yyvsp[-3].measure)), 2*GROUNDPLANEFRAME,
					OU ((yyvsp[-3].measure)) + 2*MASKFRAME, (yyvsp[-2].string),p_number, OldFlags((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2749 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 162:
#line 1712 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { (yyval.flagtype) = OldFlags((yyvsp[0].integer)); }
#line 2755 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 163:
#line 1713 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { (yyval.flagtype) = string_to_flags ((yyvsp[0].string), yyerror); free((yyvsp[0].string)); }
#line 2761 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 167:
#line 1743 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				if ((yyvsp[-3].integer) <= 0 || (yyvsp[-3].integer) > MAX_FONTPOSITION)
				{
					yyerror("fontposition out of range");
					YYABORT;
				}
				Symbol = &yyFont->Symbol[(yyvsp[-3].integer)];
				if (Symbol->Valid)
				{
					yyerror("symbol ID used twice");
					YYABORT;
				}
				Symbol->Valid = true;
				Symbol->Delta = NU ((yyvsp[-2].measure));
			}
#line 2781 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 168:
#line 1759 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				if ((yyvsp[-3].integer) <= 0 || (yyvsp[-3].integer) > MAX_FONTPOSITION)
				{
					yyerror("fontposition out of range");
					YYABORT;
				}
				Symbol = &yyFont->Symbol[(yyvsp[-3].integer)];
				if (Symbol->Valid)
				{
					yyerror("symbol ID used twice");
					YYABORT;
				}
				Symbol->Valid = true;
				Symbol->Delta = OU ((yyvsp[-2].measure));
			}
#line 2801 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 174:
#line 1806 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewLineInSymbol(Symbol, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)), OU ((yyvsp[-1].measure)));
			}
#line 2809 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 175:
#line 1813 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewLineInSymbol(Symbol, NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), NU ((yyvsp[-3].measure)), NU ((yyvsp[-2].measure)), NU ((yyvsp[-1].measure)));
			}
#line 2817 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 183:
#line 1868 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				Menu = CreateNewNet((LibraryTypePtr)&yyPCB->NetlistLib, (yyvsp[-3].string), (yyvsp[-2].string));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2827 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 189:
#line 1903 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
				CreateNewConnection(Menu, (yyvsp[-1].string));
				free ((yyvsp[-1].string));
			}
#line 2836 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 197:
#line 1950 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { rats_patch_append(yyPCB, RATP_ADD_CONN, (yyvsp[-2].string), (yyvsp[-1].string), NULL); free((yyvsp[-2].string)); free((yyvsp[-1].string)); }
#line 2842 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 198:
#line 1951 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { rats_patch_append(yyPCB, RATP_DEL_CONN, (yyvsp[-2].string), (yyvsp[-1].string), NULL); free((yyvsp[-2].string)); free((yyvsp[-1].string)); }
#line 2848 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 199:
#line 1952 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { rats_patch_append(yyPCB, RATP_CHANGE_ATTRIB, (yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].string)); free((yyvsp[-3].string)); free((yyvsp[-2].string)); free((yyvsp[-1].string)); }
#line 2854 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 200:
#line 1980 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    {
			  CreateNewAttribute (attr_list, (yyvsp[-2].string), (yyvsp[-1].string) ? (yyvsp[-1].string) : (char *)"");
				free ((yyvsp[-2].string));
				free ((yyvsp[-1].string));
			}
#line 2864 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 201:
#line 1987 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 2870 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 202:
#line 1988 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { (yyval.string) = 0; }
#line 2876 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 203:
#line 1992 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { (yyval.number) = (yyvsp[0].number); }
#line 2882 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 204:
#line 1993 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { (yyval.number) = (yyvsp[0].integer); }
#line 2888 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 205:
#line 1998 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { do_measure(&(yyval.measure), (yyvsp[0].number), MIL_TO_COORD ((yyvsp[0].number)) / 100.0, 0); }
#line 2894 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 206:
#line 1999 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { M ((yyval.measure), (yyvsp[-1].number), MIL_TO_COORD ((yyvsp[-1].number)) / 100000.0); }
#line 2900 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 207:
#line 2000 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { M ((yyval.measure), (yyvsp[-1].number), MIL_TO_COORD ((yyvsp[-1].number)) / 100.0); }
#line 2906 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 208:
#line 2001 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { M ((yyval.measure), (yyvsp[-1].number), MIL_TO_COORD ((yyvsp[-1].number))); }
#line 2912 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 209:
#line 2002 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { M ((yyval.measure), (yyvsp[-1].number), INCH_TO_COORD ((yyvsp[-1].number))); }
#line 2918 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 210:
#line 2003 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { M ((yyval.measure), (yyvsp[-1].number), MM_TO_COORD ((yyvsp[-1].number)) / 1000000.0); }
#line 2924 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 211:
#line 2004 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { M ((yyval.measure), (yyvsp[-1].number), MM_TO_COORD ((yyvsp[-1].number)) / 1000.0); }
#line 2930 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 212:
#line 2005 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { M ((yyval.measure), (yyvsp[-1].number), MM_TO_COORD ((yyvsp[-1].number))); }
#line 2936 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 213:
#line 2006 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { M ((yyval.measure), (yyvsp[-1].number), MM_TO_COORD ((yyvsp[-1].number)) * 1000.0); }
#line 2942 "parse_y.tab.c" /* yacc.c:1646  */
    break;

  case 214:
#line 2007 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1646  */
    { M ((yyval.measure), (yyvsp[-1].number), MM_TO_COORD ((yyvsp[-1].number)) * 1000000.0); }
#line 2948 "parse_y.tab.c" /* yacc.c:1646  */
    break;


#line 2952 "parse_y.tab.c" /* yacc.c:1646  */
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
#line 2010 "../../src_plugins/io_pcb/parse_y.y" /* yacc.c:1906  */


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
