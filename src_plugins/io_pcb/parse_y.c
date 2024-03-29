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


/* Substitute the variable and function names.  */
#define yyparse         pcb_parse
#define yylex           pcb_lex
#define yyerror         pcb_error
#define yydebug         pcb_debug
#define yynerrs         pcb_nerrs

#define yylval          pcb_lval
#define yychar          pcb_char

/* First part of user prologue.  */
#line 1 "parse_y.y" /* yacc.c:337  */

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2017, 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* grammar to parse ASCII input of geda/PCB description (alien format) */

#include "config.h"
#include "flag.h"
#include "board.h"
#include "conf_core.h"
#include "layer.h"
#include "data.h"
#include <librnd/core/error.h>
#include "file.h"
#include "parse_l.h"
#include "polygon.h"
#include "remove.h"
#include <librnd/poly/rtree.h>
#include "flag_str.h"
#include "obj_pinvia_therm.h"
#include "rats_patch.h"
#include "route_style.h"
#include <librnd/core/compat_misc.h>
#include "src_plugins/lib_compat_help/pstk_compat.h"
#include "netlist.h"
#include <librnd/font2/font.h>

#include "rst_parse.c"

/* frame between the groundplane and the copper or mask - noone seems
   to remember what these two are for; changing them may have unforeseen
   side effects. */
#define PCB_GROUNDPLANEFRAME        RND_MIL_TO_COORD(15)
#define PCB_MASKFRAME               RND_MIL_TO_COORD(3)

/* default inner/outer ratio for pins/vias in percent */
#define PCB_DEFAULT_DRILLINGHOLE 40

/* min difference outer-inner diameter */
#define PCB_MIN_PINORVIACOPPER RND_MIL_TO_COORD(4)

static	pcb_layer_t *Layer;
static	pcb_poly_t *Polygon;
static	rnd_glyph_t *Glyph;
static	int		pin_num;
static pcb_net_t *currnet;
static	rnd_bool			LayerFlag[PCB_MAX_LAYER + 2];
static	int	old_fmt; /* 1 if we are reading a PCB(), 0 if PCB[] */
static	unsigned char yy_intconn;

extern	char			*yytext;		/* defined by LEX */
extern	pcb_board_t *	yyPCB;
extern	pcb_data_t *	yyData;
extern	pcb_subc_t *yysubc;
extern	rnd_coord_t yysubc_ox, yysubc_oy;
extern	rnd_font_t *	yyRndFont;
extern	rnd_bool	yyFontReset;
extern	int				pcb_lineno;		/* linenumber */
extern	char			*yyfilename;	/* in this file */
extern	rnd_conf_role_t yy_settings_dest;
extern pcb_flag_t yy_pcb_flags;
extern int *yyFontkitValid;
extern int yyElemFixLayers;

static char *layer_group_string;

static pcb_attribute_list_t *attr_list;

int yyerror(const char *s);
int yylex();
static int check_file_version (int);

static void do_measure (PLMeasure *m, rnd_coord_t i, double d, int u);
#define M(r,f,d) do_measure (&(r), f, d, 1)

/* Macros for interpreting what "measure" means - integer value only,
   old units (mil), or new units (cmil).  */
#define IV(m) integer_value (m)
#define OU(m) old_units (m)
#define NU(m) new_units (m)

static int integer_value (PLMeasure m);
static rnd_coord_t old_units (PLMeasure m);
static rnd_coord_t new_units (PLMeasure m);
static pcb_flag_t pcb_flag_old(unsigned int flags);
static void load_meta_coord(const char *path, rnd_coord_t crd);
static void load_meta_float(const char *path, double val);

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

#include "parse_y.h"


#line 199 "parse_y.c" /* yacc.c:337  */
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
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "parse_y.h".  */
#ifndef YY_PCB_PARSE_Y_H_INCLUDED
# define YY_PCB_PARSE_Y_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int pcb_debug;
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

union YYSTYPE
{
#line 127 "parse_y.y" /* yacc.c:352  */

	int		integer;
	double		number;
	char		*string;
	pcb_flag_t	flagtype;
	PLMeasure	measure;

#line 301 "parse_y.c" /* yacc.c:352  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE pcb_lval;

int pcb_parse (void);

#endif /* !YY_PCB_PARSE_Y_H_INCLUDED  */



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
#define YYLAST   614

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  55
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  113
/* YYNRULES -- Number of rules.  */
#define YYNRULES  216
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  648

#define YYUNDEFTOK  2
#define YYMAXUTOK   305

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
       0,   154,   154,   155,   156,   157,   161,   161,   235,   235,
     260,   260,   279,   280,   285,   285,   305,   307,   317,   324,
     331,   341,   342,   343,   346,   354,   369,   384,   388,   392,
     395,   397,   404,   406,   414,   416,   417,   418,   422,   432,
     443,   455,   459,   464,   468,   472,   476,   485,   494,   498,
     499,   503,   504,   508,   509,   509,   510,   511,   513,   513,
     520,   524,   525,   526,   527,   528,   533,   543,   554,   564,
     574,   590,   595,   605,   604,   640,   641,   645,   646,   650,
     651,   652,   653,   654,   655,   657,   662,   663,   664,   665,
     665,   666,   670,   679,   688,   699,   708,   717,   726,   736,
     754,   784,   783,   822,   824,   829,   828,   835,   837,   842,
     846,   853,   854,   855,   856,   857,   865,   864,   883,   882,
     901,   900,   921,   919,   943,   941,   966,   967,   971,   972,
     973,   974,   975,   977,   982,   987,   992,   997,  1002,  1007,
    1007,  1011,  1012,  1016,  1017,  1018,  1019,  1020,  1022,  1028,
    1035,  1040,  1045,  1045,  1051,  1064,  1076,  1087,  1103,  1122,
    1137,  1150,  1174,  1185,  1196,  1197,  1201,  1202,  1205,  1207,
    1223,  1242,  1243,  1246,  1248,  1249,  1254,  1261,  1267,  1268,
    1272,  1277,  1278,  1282,  1283,  1289,  1288,  1300,  1301,  1305,
    1306,  1310,  1327,  1328,  1332,  1337,  1338,  1342,  1343,  1358,
    1359,  1360,  1364,  1377,  1378,  1382,  1383,  1388,  1389,  1390,
    1391,  1392,  1393,  1394,  1395,  1396,  1397
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
  "pad_1.7_format", "pad_4.3_format", "pad_newformat", "pad", "flags",
  "symbols", "symbol", "symbolhead", "symbolid", "symboldata",
  "symboldefinition", "hiressymbol", "pcbnetlist", "pcbnetdef", "nets",
  "netdefs", "net", "$@18", "connections", "conndefs", "conn",
  "pcbnetlistpatch", "pcbnetpatchdef", "netpatches", "netpatchdefs",
  "netpatch", "attribute", "opt_string", "number", "measure", YY_NULLPTR
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

#define YYPACT_NINF -423

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-423)))

#define YYTABLE_NINF -90

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     183,  -423,    17,  -423,    20,    29,  -423,    23,  -423,    44,
    -423,    31,    97,    14,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,    53,    26,    48,  -423,   147,  -423,    83,    29,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,  -423,    66,    44,  -423,
    -423,   125,    92,   124,   145,   181,   148,    35,    35,    35,
      35,  -423,   114,  -423,  -423,    84,    84,  -423,   -13,   111,
     164,   173,   199,   158,  -423,  -423,  -423,  -423,  -423,   180,
     193,   197,   210,  -423,  -423,   259,    35,    35,    35,    35,
     231,  -423,  -423,    35,    35,   200,  -423,  -423,  -423,  -423,
      35,     3,    35,    35,   206,   151,   239,   252,    35,   261,
    -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,    35,
      35,   208,   269,   271,   244,   224,    35,    35,    35,  -423,
      35,    35,    35,    35,    35,   248,   250,   317,   130,    35,
    -423,   287,    35,   192,    35,    35,   288,   281,   300,    35,
      35,   304,   311,    35,    35,    35,    35,    35,   315,   342,
      35,    35,    35,   383,   336,    35,   388,   198,    35,    35,
    -423,  -423,  -423,    35,    35,  -423,  -423,   389,    -1,    35,
      35,   344,    35,   346,   380,  -423,  -423,  -423,    35,    35,
      35,   348,  -423,    35,   349,   394,   207,   400,   401,    35,
      35,   354,   353,  -423,   356,   355,  -423,   358,    35,   359,
     384,    35,    35,    35,   360,   213,   408,  -423,   361,   412,
     411,   145,   414,    35,    35,  -423,  -423,  -423,  -423,  -423,
      35,   202,   366,   392,    35,    35,   417,  -423,   217,   223,
     369,   275,   370,   371,   213,  -423,    83,  -423,  -423,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,  -423,   145,  -423,   372,
     420,   378,   375,   379,   381,    35,   382,   385,   428,   276,
     415,    35,    90,   386,   102,    35,    35,    35,    35,    35,
      35,    35,   145,  -423,  -423,  -423,   390,  -423,   387,  -423,
    -423,  -423,  -423,     8,  -423,  -423,   391,   432,   437,   238,
    -423,    35,   393,    35,   395,   279,   404,   405,   280,   296,
       0,  -423,    83,  -423,  -423,  -423,  -423,  -423,    35,    35,
      35,    35,    35,    35,    35,   406,  -423,  -423,  -423,    11,
    -423,   407,   409,   413,   145,   416,   442,  -423,    35,    35,
      35,    35,    35,    35,    35,    35,  -423,  -423,  -423,    35,
      35,    35,    35,    35,    35,    35,   418,  -423,    35,  -423,
    -423,   419,   427,  -423,   410,  -423,   422,   102,    35,    35,
      35,    35,    35,    35,    35,    35,    35,    35,    35,    35,
      35,    35,   226,  -423,   421,   423,   425,  -423,  -423,   426,
     102,   429,   143,    35,    35,    35,    35,    35,    35,   439,
     438,    35,    35,    35,    35,   461,   464,   470,   474,   301,
    -423,   440,   441,  -423,   332,  -423,  -423,    35,    35,   230,
      35,    35,    35,  -423,  -423,    35,    35,    35,    35,   443,
     145,   444,   477,    35,    35,  -423,   301,   462,   446,    33,
    -423,    33,    35,    35,   492,   494,    35,    35,    35,   145,
       5,    35,    35,  -423,   449,  -423,   448,    35,    35,   -17,
    -423,   450,   459,   462,  -423,   -15,   320,   321,   326,   327,
     121,  -423,    83,  -423,  -423,  -423,  -423,  -423,   155,   452,
     460,   463,   284,   509,    35,    35,   466,   465,  -423,    35,
     110,  -423,  -423,   468,   467,   469,  -423,  -423,   510,  -423,
    -423,   471,   480,   481,   482,   -15,  -423,    35,    35,    35,
      35,    35,    35,    35,    35,  -423,  -423,  -423,  -423,  -423,
    -423,  -423,   483,   519,   377,    35,    35,  -423,  -423,   145,
     484,   531,  -423,  -423,  -423,   539,   542,   543,   544,  -423,
    -423,    35,    35,    35,    35,    35,    35,    35,    35,  -423,
     501,   504,   555,   508,   507,   511,  -423,   515,   301,   516,
     556,   558,   565,    35,    35,    35,    35,    35,    35,    35,
      35,  -423,  -423,   518,  -423,  -423,  -423,  -423,   520,   522,
     523,   524,   567,    35,    35,    35,    35,    35,    35,    35,
      35,  -423,  -423,  -423,  -423,  -423,   525,    35,    35,    35,
      35,    35,    35,    35,    35,   547,  -423,   529,   528,    35,
      35,    35,    35,    35,    35,   530,   532,   547,  -423,  -423,
    -423,   570,   578,   237,    35,    35,    35,   579,  -423,  -423,
     581,   582,   583,   584,   585,   540,   541,   545,   145,   589,
     145,   588,   590,  -423,  -423,  -423,   546,   548,   549,   145,
     593,  -423,  -423,  -423,   551,   550,  -423,  -423
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     5,     0,     2,    16,     0,     3,     0,     4,     0,
       1,     0,     0,     0,     9,   111,   112,   113,   114,   115,
      60,     0,     0,     0,    11,     0,    51,     0,     0,    53,
      61,    62,    63,    64,    65,    56,    57,     0,    15,   166,
     173,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    52,     0,    55,    59,     0,     0,   167,     0,     0,
       0,     0,     0,    29,    21,    22,    23,   164,   165,     0,
       0,     0,     0,   205,   206,   207,     0,     0,     0,     0,
       0,   171,   172,     0,     0,     0,   168,   174,   175,    17,
       0,     0,     0,     0,     0,    30,     0,     0,     0,   204,
     208,   209,   210,   211,   212,   213,   214,   215,   216,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    18,
       0,     0,     0,     0,     0,     0,    32,     0,     0,     0,
     203,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    34,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     202,   169,   170,     0,     0,    20,    19,     0,     0,     0,
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
      72,   177,   176,     0,    41,    42,     0,     0,     0,     0,
      12,     0,   206,     0,     0,     0,     0,     0,     0,     0,
     139,   126,     0,   128,   129,   130,   131,   132,     0,     0,
       0,     0,     0,     0,     0,     0,    66,    67,    38,     0,
      44,     0,     0,   179,     0,     0,     0,   118,     0,     0,
       0,     0,     0,     0,     0,     0,   117,   127,   140,     0,
       0,     0,     0,     0,     0,     0,     0,    39,     0,    47,
      46,     0,   193,   178,     0,   120,     0,   139,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   101,     0,     0,     0,     7,   192,     0,
     139,     0,   139,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
      40,     0,     0,   124,   139,   122,   119,     0,     0,     0,
       0,     0,     0,   137,   138,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   103,   107,   182,     0,   152,
     121,   152,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    85,     0,    98,     0,     0,     0,     0,
     108,     0,     0,   181,   183,   196,     0,     0,     0,     0,
     152,   141,     0,   144,   143,   147,   145,   146,   152,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    94,     0,
       0,   100,    99,     0,     0,     0,   102,   104,     0,   180,
     184,     0,     0,     0,     0,   195,   197,     0,     0,     0,
       0,     0,     0,     0,     0,   125,   142,   153,   123,   133,
     134,   158,     0,     0,     0,     0,     0,    92,    93,     0,
     206,     0,   110,   109,   105,     0,     0,     0,     0,   194,
     198,     0,     0,     0,     0,     0,     0,     0,     0,   157,
       0,     0,     0,     0,     0,     0,    97,     0,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   156,   163,     0,   135,   136,    95,    96,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   162,   106,   185,   199,   200,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   188,   201,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   187,   189,   148,
     149,     0,     0,     0,     0,     0,     0,     0,   186,   190,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   150,   151,   191,     0,     0,     0,     0,
       0,   154,   155,   161,     0,     0,   159,   160
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,   340,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,   316,  -423,   586,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,  -423,   373,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -422,  -423,
     580,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,  -302,  -292,  -423,   175,  -412,  -423,  -423,  -423,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,  -423,  -164,  -423,   571,
    -423,   554,  -423,  -423,  -423,  -423,  -423,  -423,  -423,   159,
    -423,  -423,  -423,     6,  -423,  -423,  -423,  -423,   119,  -231,
    -423,   -47,   -48
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     4,     5,     6,     7,   289,     8,     9,
      12,    43,    63,    64,    65,    66,    95,   126,   149,   174,
     175,   176,   177,   200,   223,   260,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,   205,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   399,   449,   487,   548,   425,   426,
      14,    15,   264,    16,   357,    17,   380,    18,   431,    19,
     429,   300,   301,   302,   460,   461,   462,   463,   464,   303,
     304,   305,   465,   466,   467,   306,   307,    69,    38,    39,
      40,    83,    58,    87,    88,   352,   353,   452,   453,   454,
     595,   606,   607,   608,   377,   378,   494,   495,   496,    53,
     131,    75,    76
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      77,    78,    79,   192,   450,   275,    73,    74,   337,   477,
      85,    73,    74,   485,    73,    74,   295,    10,   296,   297,
     491,   492,   493,   -50,    20,   298,   299,    11,   109,   110,
     111,   112,    21,    22,    23,   114,   115,   486,    73,    74,
     -58,    86,   118,   120,   121,   122,    13,   251,   506,   456,
     129,   457,   458,   193,   336,   382,   506,   119,   459,   478,
     318,   132,   133,   347,   -54,    44,    37,    45,   139,   140,
     141,   338,   142,   143,   144,   145,   146,    47,   404,    48,
     152,   153,    41,   276,   155,   157,   158,   159,    81,    82,
     337,   163,   164,    73,   292,   167,   168,   169,   170,    49,
     171,    50,   178,   179,   180,    42,    46,   183,   315,   186,
     187,   188,   337,    73,   520,   189,   190,    55,   295,    56,
     296,   297,   194,   195,    52,   197,   568,   298,   299,    59,
     201,   202,   203,    73,    74,   206,   151,   456,   210,   457,
     458,   213,   214,    60,    62,    61,   459,   -49,    20,    67,
     220,    68,    72,   224,   225,   226,    21,    22,    23,   295,
     354,   296,   297,    89,   -58,   253,   254,    80,   298,   299,
      90,   456,   255,   457,   458,   505,   261,   262,   -49,    91,
     459,   -49,    94,   -10,     1,    70,    96,    71,   -54,   125,
      -6,    -6,   -10,   -10,   -10,    73,    74,   406,   156,    97,
      -8,    73,    74,    98,   185,   -14,   256,   283,   257,   508,
      73,    74,   134,   209,   291,   293,    99,   308,   309,   310,
     311,   312,   313,   314,   -10,   228,   229,   230,   231,    73,
      74,   507,   397,    73,    74,   319,   434,   113,   -50,    20,
      73,    74,   232,   622,   324,   127,   326,    21,    22,    23,
      92,   116,    93,   117,   -89,   -58,   444,   123,   128,   124,
     339,   340,   341,   342,   343,   344,   345,   130,   265,   -50,
     266,   348,   -50,   135,   267,   476,   268,   136,   138,   -54,
     358,   359,   360,   361,   362,   363,   364,   365,   512,   148,
     513,   366,   367,   368,   369,   370,   137,   371,   372,   147,
     374,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     383,   384,   385,   386,   387,   388,   389,   390,   391,   392,
     393,   394,   395,   150,   396,   398,   270,   287,   271,   288,
     328,   332,   329,   333,   161,   407,   408,   409,   410,   411,
     412,   154,   160,   415,   416,   417,   418,   334,   295,   335,
     296,   297,   423,   162,   424,   545,   165,   298,   299,   432,
     433,   435,   436,   437,   438,   166,   172,   439,   440,   441,
     442,   497,   499,   498,   500,   447,   448,   501,   503,   502,
     504,   541,   173,   542,   469,   470,   430,   181,   473,   182,
     474,   475,   184,   191,   479,   480,   196,   198,   208,   483,
     484,   199,   204,   207,   211,   212,   215,   216,   217,   218,
     219,   222,   221,   227,   247,   248,   249,   250,   252,   258,
     259,   263,   269,   272,   278,   273,   277,   515,   516,   280,
     279,   281,   519,   521,   286,   282,   284,   -14,   321,   285,
     294,   317,   316,   322,   351,   320,   356,   325,   327,   531,
     532,   533,   534,   535,   536,   537,   538,   330,   331,   349,
     346,   376,   379,   350,   636,   419,   638,   543,   544,   355,
     420,   373,   375,   400,   421,   644,   381,   401,   402,   403,
     422,   446,   405,   553,   554,   555,   556,   557,   558,   559,
     560,   413,   414,   427,   451,   428,   471,   443,   445,   455,
     472,   481,   482,   488,   509,   573,   574,   575,   576,   577,
     578,   579,   580,   489,   510,   514,   525,   511,   517,   518,
     522,   523,   524,   540,   526,   587,   588,   589,   590,   591,
     592,   593,   594,   527,   528,   547,   529,   539,   546,   597,
     598,   599,   600,   601,   602,   549,   603,   604,   550,   551,
     552,   611,   612,   613,   614,   561,   615,   616,   562,   563,
     564,   565,   570,   566,   571,   623,   624,   625,   626,   567,
     569,   572,   581,   586,   582,   583,   620,   584,   585,   596,
     605,   609,   610,   617,   621,   627,   618,   628,   629,   630,
     631,   632,   633,   637,   639,   634,   640,   645,   641,   635,
     290,   643,   642,   646,   647,   323,   468,   274,    54,    57,
      84,    51,   490,   619,   530
};

static const yytype_uint16 yycheck[] =
{
      48,    49,    50,     4,   426,   236,     3,     4,   300,     4,
      23,     3,     4,    30,     3,     4,    16,     0,    18,    19,
      35,    36,    37,     0,     1,    25,    26,     7,    76,    77,
      78,    79,     9,    10,    11,    83,    84,    54,     3,     4,
      17,    54,    90,    91,    92,    93,    17,   211,   460,    16,
      98,    18,    19,    54,    54,   357,   468,    54,    25,    54,
      52,   109,   110,    52,    41,    51,    22,    53,   116,   117,
     118,   302,   120,   121,   122,   123,   124,    51,   380,    53,
     128,   129,    51,   247,   132,   133,   134,   135,     4,     5,
     382,   139,   140,     3,     4,   143,   144,   145,   146,    51,
     147,    53,   150,   151,   152,     8,    53,   155,   272,   157,
     158,   159,   404,     3,     4,   163,   164,    51,    16,    53,
      18,    19,   169,   170,    41,   172,   548,    25,    26,     4,
     178,   179,   180,     3,     4,   183,     6,    16,   186,    18,
      19,   189,   190,    51,    20,    53,    25,     0,     1,     4,
     198,     6,     4,   201,   202,   203,     9,    10,    11,    16,
     324,    18,    19,    52,    17,   213,   214,    53,    25,    26,
       6,    16,   220,    18,    19,    54,   224,   225,    31,     6,
      25,    34,    24,     0,     1,     4,     6,     6,    41,    38,
       7,     8,     9,    10,    11,     3,     4,    54,     6,     6,
      17,     3,     4,     6,     6,    22,     4,   255,     6,    54,
       3,     4,     4,     6,   261,   262,     6,   265,   266,   267,
     268,   269,   270,   271,    41,    12,    13,    14,    15,     3,
       4,   462,     6,     3,     4,   283,     6,     6,     0,     1,
       3,     4,    29,     6,   291,     6,   293,     9,    10,    11,
      51,    51,    53,    53,    41,    17,   420,    51,     6,    53,
     308,   309,   310,   311,   312,   313,   314,     6,    51,    31,
      53,   319,    34,     4,    51,   439,    53,     6,    54,    41,
     328,   329,   330,   331,   332,   333,   334,   335,     4,    39,
       6,   339,   340,   341,   342,   343,    52,   344,   345,    51,
     348,    42,    43,    44,    45,    46,    47,    48,    49,    50,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,     6,   371,   372,    51,    51,    53,    53,
      51,    51,    53,    53,    53,   383,   384,   385,   386,   387,
     388,    54,    54,   391,   392,   393,   394,    51,    16,    53,
      18,    19,    51,    53,    53,   519,    52,    25,    26,   407,
     408,   409,   410,   411,   412,    54,    51,   415,   416,   417,
     418,    51,    51,    53,    53,   423,   424,    51,    51,    53,
      53,     4,    40,     6,   432,   433,    54,     4,   436,    53,
     437,   438,     4,     4,   441,   442,    52,    51,     4,   447,
     448,    21,    54,    54,     4,     4,    52,    54,    52,    54,
      52,    27,    53,    53,     6,    54,     4,     6,     4,    53,
      28,     4,    53,    53,     4,    54,    54,   474,   475,    54,
      52,    52,   479,   480,     6,    54,    54,    22,     6,    54,
      54,    54,    52,     6,    31,    54,     4,    54,    53,   497,
     498,   499,   500,   501,   502,   503,   504,    53,    53,    52,
      54,    34,    52,    54,   628,     4,   630,   515,   516,    53,
       6,    53,    53,    52,     4,   639,    54,    54,    53,    53,
       6,     4,    53,   531,   532,   533,   534,   535,   536,   537,
     538,    52,    54,    53,    32,    54,     4,    54,    54,    53,
       6,    52,    54,    53,    52,   553,   554,   555,   556,   557,
     558,   559,   560,    54,    54,     6,     6,    54,    52,    54,
      52,    54,    53,     4,    53,   573,   574,   575,   576,   577,
     578,   579,   580,    53,    53,     4,    54,    54,    54,   587,
     588,   589,   590,   591,   592,     6,   593,   594,     6,     6,
       6,   599,   600,   601,   602,    54,   603,   604,    54,     4,
      52,    54,     6,    52,     6,   613,   614,   615,   616,    54,
      54,     6,    54,     6,    54,    53,     6,    54,    54,    54,
      33,    52,    54,    53,     6,     6,    54,     6,     6,     6,
       6,     6,    52,     4,     6,    54,     6,     4,    52,    54,
     260,    52,    54,    52,    54,   289,   431,   234,    28,    38,
      56,    25,   453,   607,   495
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    56,    57,    58,    59,    60,    61,    63,    64,
       0,     7,    65,    17,   115,   116,   118,   120,   122,   124,
       1,     9,    10,    11,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    22,   143,   144,
     145,    51,     8,    66,    51,    53,    53,    51,    53,    51,
      53,    83,    41,   164,   115,    51,    53,   144,   147,     4,
      51,    53,    20,    67,    68,    69,    70,     4,     6,   142,
       4,     6,     4,     3,     4,   166,   167,   167,   167,   167,
      53,     4,     5,   146,   146,    23,    54,   148,   149,    52,
       6,     6,    51,    53,    24,    71,     6,     6,     6,     6,
      42,    43,    44,    45,    46,    47,    48,    49,    50,   167,
     167,   167,   167,     6,   167,   167,    51,    53,   167,    54,
     167,   167,   167,    51,    53,    38,    72,     6,     6,   167,
       6,   165,   167,   167,     4,     4,     6,    52,    54,   167,
     167,   167,   167,   167,   167,   167,   167,    51,    39,    73,
       6,     6,   167,   167,    54,   167,     6,   167,   167,   167,
      54,    53,    53,   167,   167,    52,    54,   167,   167,   167,
     167,   166,    51,    40,    74,    75,    76,    77,   167,   167,
     167,     4,    53,   167,     4,     6,   167,   167,   167,   167,
     167,     4,     4,    54,   166,   166,    52,   166,    51,    21,
      78,   167,   167,   167,    54,    94,   167,    54,     4,     6,
     167,     4,     4,   167,   167,    52,    54,    52,    54,    52,
     167,    53,    27,    79,   167,   167,   167,    53,    12,    13,
      14,    15,    29,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,     6,    54,     4,
       6,   142,     4,   167,   167,   167,     4,     6,    53,    28,
      80,   167,   167,     4,   117,    51,    53,    51,    53,    53,
      51,    53,    53,    54,    97,   164,   142,    54,     4,    52,
      54,    52,    54,   167,    54,    54,     6,    51,    53,    62,
      63,   166,     4,   166,    54,    16,    18,    19,    25,    26,
     126,   127,   128,   134,   135,   136,   140,   141,   167,   167,
     167,   167,   167,   167,   167,   142,    52,    54,    52,   167,
      54,     6,     6,    81,   166,    54,   166,    53,    51,    53,
      53,    53,    51,    53,    51,    53,    54,   127,   164,   167,
     167,   167,   167,   167,   167,   167,    54,    52,   167,    52,
      54,    31,   150,   151,   142,    53,     4,   119,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   166,   166,    53,   167,    53,    34,   159,   160,    52,
     121,    54,   126,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   166,     6,   166,   109,
      52,    54,    53,    53,   126,    53,    54,   167,   167,   167,
     167,   167,   167,    52,    54,   167,   167,   167,   167,     4,
       6,     4,     6,    51,    53,   113,   114,    53,    54,   125,
      54,   123,   167,   167,     6,   167,   167,   167,   167,   167,
     167,   167,   167,    54,   142,    54,     4,   167,   167,   110,
     113,    32,   152,   153,   154,    53,    16,    18,    19,    25,
     129,   130,   131,   132,   133,   137,   138,   139,   129,   167,
     167,     4,     6,   167,   166,   166,   142,     4,    54,   166,
     166,    52,    54,   167,   167,    30,    54,   111,    53,    54,
     154,    35,    36,    37,   161,   162,   163,    51,    53,    51,
      53,    51,    53,    51,    53,    54,   130,   164,    54,    52,
      54,    54,     4,     6,     6,   166,   166,    52,    54,   166,
       4,   166,    52,    54,    53,     6,    53,    53,    53,    54,
     163,   167,   167,   167,   167,   167,   167,   167,   167,    54,
       4,     4,     6,   167,   167,   142,    54,     4,   112,     6,
       6,     6,     6,   167,   167,   167,   167,   167,   167,   167,
     167,    54,    54,     4,    52,    54,    52,    54,   113,    54,
       6,     6,     6,   167,   167,   167,   167,   167,   167,   167,
     167,    54,    54,    53,    54,    54,     6,   167,   167,   167,
     167,   167,   167,   167,   167,   155,    54,   167,   167,   167,
     167,   167,   167,   166,   166,    33,   156,   157,   158,    52,
      54,   167,   167,   167,   167,   166,   166,    53,    54,   158,
       6,     6,     6,   167,   167,   167,   167,     6,     6,     6,
       6,     6,     6,    52,    54,    54,   142,     4,   142,     6,
       6,    52,    54,    52,   142,     4,    52,    54
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
     130,   130,   131,   130,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   142,   143,   143,   144,   145,
     145,   146,   146,   147,   147,   147,   148,   149,   150,   150,
     151,   152,   152,   153,   153,   155,   154,   156,   156,   157,
     157,   158,   159,   159,   160,   161,   161,   162,   162,   163,
     163,   163,   164,   165,   165,   166,   166,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167
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
       2,     1,     2,     1,     1,     1,     1,     1,     8,     8,
      10,    10,     0,     2,    12,    12,    10,     9,     8,    13,
      13,    12,    11,    10,     1,     1,     1,     2,     3,     6,
       6,     1,     1,     0,     2,     2,     8,     8,     1,     0,
       6,     1,     0,     1,     2,     0,     9,     1,     0,     1,
       2,     4,     1,     0,     6,     1,     0,     1,     2,     5,
       5,     6,     5,     1,     0,     1,     1,     1,     2,     2,
       2,     2,     2,     2,     2,     2,     2
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
        case 5:
#line 157 "parse_y.y" /* yacc.c:1652  */
    { YYABORT; }
#line 1820 "parse_y.c" /* yacc.c:1652  */
    break;

  case 6:
#line 161 "parse_y.y" /* yacc.c:1652  */
    {
					/* reset flags for 'used layers';
					 * init font and data pointers
					 */
				int	i;

				if (!yyPCB)
				{
					rnd_message(RND_MSG_ERROR, "illegal fileformat\n");
					YYABORT;
				}
				for (i = 0; i < PCB_MAX_LAYER + 2; i++)
					LayerFlag[i] = rnd_false;
				yyRndFont = &yyPCB->fontkit.dflt;
				yyFontkitValid = &yyPCB->fontkit.valid;
				yyData = yyPCB->Data;
				PCB_SET_PARENT(yyData, board, yyPCB);
				yyData->LayerN = 0;
				yyPCB->NetlistPatches = yyPCB->NetlistPatchLast = NULL;
				layer_group_string = NULL;
				old_fmt = 0;
			}
#line 1847 "parse_y.c" /* yacc.c:1652  */
    break;

  case 7:
#line 197 "parse_y.y" /* yacc.c:1652  */
    {
			  pcb_board_t *pcb_save = PCB;
			  if ((yy_settings_dest != RND_CFR_invalid) && (layer_group_string != NULL))
					rnd_conf_set(yy_settings_dest, "design/groups", -1, layer_group_string, RND_POL_OVERWRITE);
			  pcb_board_new_postproc(yyPCB, 0);
			  if (layer_group_string == NULL) {
			     if (pcb_layer_improvise(yyPCB, rnd_true) != 0) {
			        rnd_message(RND_MSG_ERROR, "missing layer-group string, failed to improvise the groups\n");
			        YYABORT;
			     }
			     rnd_message(RND_MSG_ERROR, "missing layer-group string: invalid input file, had to improvise, the layer stack is most probably broken\n");
			  }
			  else {
			    if (pcb_layer_parse_group_string(yyPCB, layer_group_string, yyData->LayerN, old_fmt))
			    {
			      rnd_message(RND_MSG_ERROR, "illegal layer-group string\n");
			      YYABORT;
			    }
			    else {
			     if (pcb_layer_improvise(yyPCB, rnd_false) != 0) {
			        rnd_message(RND_MSG_ERROR, "failed to extend-improvise the groups\n");
			        YYABORT;
			     }
			    }
			  }
			/* initialize the polygon clipping now since
			 * we didn't know the layer grouping before.
			 */
			free(layer_group_string);
			PCB = yyPCB;
			PCB_POLY_ALL_LOOP(yyData);
			{
			  pcb_poly_init_clip(yyData, layer, polygon);
			}
			PCB_ENDALL_LOOP;
			PCB = pcb_save;
			}
#line 1889 "parse_y.c" /* yacc.c:1652  */
    break;

  case 8:
#line 235 "parse_y.y" /* yacc.c:1652  */
    { PreLoadElementPCB ();
		    layer_group_string = NULL; }
#line 1896 "parse_y.c" /* yacc.c:1652  */
    break;

  case 9:
#line 238 "parse_y.y" /* yacc.c:1652  */
    { LayerFlag[0] = rnd_true;
		    LayerFlag[1] = rnd_true;
		    if (yyElemFixLayers) {
		    	yyData->LayerN = 2;
		    	free((char *)yyData->Layer[0].name);
		    	yyData->Layer[0].name = rnd_strdup("top-silk");
		    	yyData->Layer[0].parent_type = PCB_PARENT_DATA;
		    	yyData->Layer[0].parent.data = yyData;
		    	yyData->Layer[0].is_bound = 1;
		    	yyData->Layer[0].meta.bound.type = PCB_LYT_SILK | PCB_LYT_TOP;
		    	free((char *)yyData->Layer[1].name);
		    	yyData->Layer[1].name = rnd_strdup("bottom-silk");
		    	yyData->Layer[1].parent_type = PCB_PARENT_DATA;
		    	yyData->Layer[1].parent.data = yyData;
		    	yyData->Layer[1].is_bound = 1;
		    	yyData->Layer[1].meta.bound.type = PCB_LYT_SILK | PCB_LYT_BOTTOM;
		    }
		    PostLoadElementPCB ();
		  }
#line 1920 "parse_y.c" /* yacc.c:1652  */
    break;

  case 10:
#line 260 "parse_y.y" /* yacc.c:1652  */
    {
					/* reset flags for 'used layers';
					 * init font and data pointers
					 */
				int	i;

				if (!yyData || !yyRndFont)
				{
					rnd_message(RND_MSG_ERROR, "illegal fileformat\n");
					YYABORT;
				}
				for (i = 0; i < PCB_MAX_LAYER + 2; i++)
					LayerFlag[i] = rnd_false;
				yyData->LayerN = 0;
			}
#line 1940 "parse_y.c" /* yacc.c:1652  */
    break;

  case 14:
#line 285 "parse_y.y" /* yacc.c:1652  */
    {
					/* mark all symbols invalid */
				if (!yyRndFont)
				{
					rnd_message(RND_MSG_ERROR, "illegal fileformat\n");
					YYABORT;
				}
				if (yyFontReset) {
					rnd_font_free(yyRndFont);
					yyRndFont->id = 0;
				}
				*yyFontkitValid = rnd_false;
			}
#line 1958 "parse_y.c" /* yacc.c:1652  */
    break;

  case 15:
#line 299 "parse_y.y" /* yacc.c:1652  */
    {
				*yyFontkitValid = rnd_true;
				rnd_font_normalize_pcb_rnd(yyRndFont);
			}
#line 1967 "parse_y.c" /* yacc.c:1652  */
    break;

  case 17:
#line 308 "parse_y.y" /* yacc.c:1652  */
    {
  if (check_file_version ((yyvsp[-1].integer)) != 0)
    {
      YYABORT;
    }
}
#line 1978 "parse_y.c" /* yacc.c:1652  */
    break;

  case 18:
#line 318 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->hidlib.name = (yyvsp[-1].string);
				yyPCB->hidlib.dwg.X2 = RND_MAX_COORD;
				yyPCB->hidlib.dwg.Y2 = RND_MAX_COORD;
				old_fmt = 1;
			}
#line 1989 "parse_y.c" /* yacc.c:1652  */
    break;

  case 19:
#line 325 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->hidlib.name = (yyvsp[-3].string);
				yyPCB->hidlib.dwg.X2 = OU ((yyvsp[-2].measure));
				yyPCB->hidlib.dwg.Y2 = OU ((yyvsp[-1].measure));
				old_fmt = 1;
			}
#line 2000 "parse_y.c" /* yacc.c:1652  */
    break;

  case 20:
#line 332 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->hidlib.name = (yyvsp[-3].string);
				yyPCB->hidlib.dwg.X2 = NU ((yyvsp[-2].measure));
				yyPCB->hidlib.dwg.Y2 = NU ((yyvsp[-1].measure));
				old_fmt = 0;
			}
#line 2011 "parse_y.c" /* yacc.c:1652  */
    break;

  case 24:
#line 347 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->hidlib.grid = OU ((yyvsp[-3].measure));
				yyPCB->hidlib.grid_ox = OU ((yyvsp[-2].measure));
				yyPCB->hidlib.grid_oy = OU ((yyvsp[-1].measure));
			}
#line 2021 "parse_y.c" /* yacc.c:1652  */
    break;

  case 25:
#line 355 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->hidlib.grid = OU ((yyvsp[-4].measure));
				yyPCB->hidlib.grid_ox = OU ((yyvsp[-3].measure));
				yyPCB->hidlib.grid_oy = OU ((yyvsp[-2].measure));
				if (yy_settings_dest != RND_CFR_invalid) {
					if ((yyvsp[-1].integer))
						rnd_conf_set(yy_settings_dest, "editor/draw_grid", -1, "true", RND_POL_OVERWRITE);
					else
						rnd_conf_set(yy_settings_dest, "editor/draw_grid", -1, "false", RND_POL_OVERWRITE);
				}
			}
#line 2037 "parse_y.c" /* yacc.c:1652  */
    break;

  case 26:
#line 370 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->hidlib.grid = NU ((yyvsp[-4].measure));
				yyPCB->hidlib.grid_ox = NU ((yyvsp[-3].measure));
				yyPCB->hidlib.grid_oy = NU ((yyvsp[-2].measure));
				if (yy_settings_dest != RND_CFR_invalid) {
					if ((yyvsp[-1].integer))
						rnd_conf_set(yy_settings_dest, "editor/draw_grid", -1, "true", RND_POL_OVERWRITE);
					else
						rnd_conf_set(yy_settings_dest, "editor/draw_grid", -1, "false", RND_POL_OVERWRITE);
				}
			}
#line 2053 "parse_y.c" /* yacc.c:1652  */
    break;

  case 27:
#line 385 "parse_y.y" /* yacc.c:1652  */
    {
/* Not loading cursor position and zoom anymore */
			}
#line 2061 "parse_y.c" /* yacc.c:1652  */
    break;

  case 28:
#line 389 "parse_y.y" /* yacc.c:1652  */
    {
/* Not loading cursor position and zoom anymore */
			}
#line 2069 "parse_y.c" /* yacc.c:1652  */
    break;

  case 31:
#line 398 "parse_y.y" /* yacc.c:1652  */
    {
				/* Read in cmil^2 for now; in future this should be a noop. */
				load_meta_float("design/poly_isle_area", RND_MIL_TO_COORD(RND_MIL_TO_COORD ((yyvsp[-1].number)) / 100.0) / 100.0);
			}
#line 2078 "parse_y.c" /* yacc.c:1652  */
    break;

  case 33:
#line 407 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->ThermScale = (yyvsp[-1].number);
				if (yyPCB->ThermScale < 0.01)
					rnd_message(RND_MSG_ERROR, "Your ThermalScale is too small. This will probably cause problems in calculating thermals.\n");
			}
#line 2088 "parse_y.c" /* yacc.c:1652  */
    break;

  case 38:
#line 423 "parse_y.y" /* yacc.c:1652  */
    {
				load_meta_coord("design/bloat", NU((yyvsp[-3].measure)));
				load_meta_coord("design/shrink", NU((yyvsp[-2].measure)));
				load_meta_coord("design/min_wid", NU((yyvsp[-1].measure)));
				load_meta_coord("design/min_ring", NU((yyvsp[-1].measure)));
			}
#line 2099 "parse_y.c" /* yacc.c:1652  */
    break;

  case 39:
#line 433 "parse_y.y" /* yacc.c:1652  */
    {
				load_meta_coord("design/bloat", NU((yyvsp[-4].measure)));
				load_meta_coord("design/shrink", NU((yyvsp[-3].measure)));
				load_meta_coord("design/min_wid", NU((yyvsp[-2].measure)));
				load_meta_coord("design/min_slk", NU((yyvsp[-1].measure)));
				load_meta_coord("design/min_ring", NU((yyvsp[-2].measure)));
			}
#line 2111 "parse_y.c" /* yacc.c:1652  */
    break;

  case 40:
#line 444 "parse_y.y" /* yacc.c:1652  */
    {
				load_meta_coord("design/bloat", NU((yyvsp[-6].measure)));
				load_meta_coord("design/shrink", NU((yyvsp[-5].measure)));
				load_meta_coord("design/min_wid", NU((yyvsp[-4].measure)));
				load_meta_coord("design/min_slk", NU((yyvsp[-3].measure)));
				load_meta_coord("design/min_drill", NU((yyvsp[-2].measure)));
				load_meta_coord("design/min_ring", NU((yyvsp[-1].measure)));
			}
#line 2124 "parse_y.c" /* yacc.c:1652  */
    break;

  case 41:
#line 456 "parse_y.y" /* yacc.c:1652  */
    {
				yy_pcb_flags = pcb_flag_make((yyvsp[-1].integer) & PCB_FLAGS);
			}
#line 2132 "parse_y.c" /* yacc.c:1652  */
    break;

  case 42:
#line 460 "parse_y.y" /* yacc.c:1652  */
    {
			  yy_pcb_flags = pcb_strflg_board_s2f((yyvsp[-1].string), yyerror);
			  free((yyvsp[-1].string));
			}
#line 2141 "parse_y.c" /* yacc.c:1652  */
    break;

  case 44:
#line 469 "parse_y.y" /* yacc.c:1652  */
    {
			  layer_group_string = (yyvsp[-1].string);
			}
#line 2149 "parse_y.c" /* yacc.c:1652  */
    break;

  case 46:
#line 477 "parse_y.y" /* yacc.c:1652  */
    {
				if (pcb_route_string_parse(yyPCB->Data, (yyvsp[-1].string), &yyPCB->RouteStyle, "mil"))
				{
					rnd_message(RND_MSG_ERROR, "illegal route-style string\n");
					YYABORT;
				}
				free((yyvsp[-1].string));
			}
#line 2162 "parse_y.c" /* yacc.c:1652  */
    break;

  case 47:
#line 486 "parse_y.y" /* yacc.c:1652  */
    {
				if (pcb_route_string_parse(yyPCB->Data, ((yyvsp[-1].string) == NULL ? "" : (yyvsp[-1].string)), &yyPCB->RouteStyle, "cmil"))
				{
					rnd_message(RND_MSG_ERROR, "illegal route-style string\n");
					YYABORT;
				}
				free((yyvsp[-1].string));
			}
#line 2175 "parse_y.c" /* yacc.c:1652  */
    break;

  case 54:
#line 509 "parse_y.y" /* yacc.c:1652  */
    { attr_list = & yyPCB->Attributes; }
#line 2181 "parse_y.c" /* yacc.c:1652  */
    break;

  case 58:
#line 513 "parse_y.y" /* yacc.c:1652  */
    {
					/* clear pointer to force memory allocation by
					 * the appropriate subroutine
					 */
				yysubc = NULL;
			}
#line 2192 "parse_y.c" /* yacc.c:1652  */
    break;

  case 60:
#line 520 "parse_y.y" /* yacc.c:1652  */
    { YYABORT; }
#line 2198 "parse_y.c" /* yacc.c:1652  */
    break;

  case 66:
#line 534 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_old_via_new(yyData, -1, NU ((yyvsp[-8].measure)), NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)),
				                     NU ((yyvsp[-3].measure)), (yyvsp[-2].string), (yyvsp[-1].flagtype));
				free ((yyvsp[-2].string));
			}
#line 2208 "parse_y.c" /* yacc.c:1652  */
    break;

  case 67:
#line 544 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_old_via_new(yyData, -1, OU ((yyvsp[-8].measure)), OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-2].string),
					pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2218 "parse_y.c" /* yacc.c:1652  */
    break;

  case 68:
#line 555 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_old_via_new(yyData, -1, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)),
					     OU ((yyvsp[-5].measure)) + OU((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2228 "parse_y.c" /* yacc.c:1652  */
    break;

  case 69:
#line 565 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_old_via_new(yyData, -1, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU((yyvsp[-4].measure)) + 2*PCB_MASKFRAME,  OU ((yyvsp[-3].measure)), (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2238 "parse_y.c" /* yacc.c:1652  */
    break;

  case 70:
#line 575 "parse_y.y" /* yacc.c:1652  */
    {
				rnd_coord_t	hole = (OU((yyvsp[-3].measure)) * PCB_DEFAULT_DRILLINGHOLE);

					/* make sure that there's enough copper left */
				if (OU((yyvsp[-3].measure)) - hole < PCB_MIN_PINORVIACOPPER &&
					OU((yyvsp[-3].measure)) > PCB_MIN_PINORVIACOPPER)
					hole = OU((yyvsp[-3].measure)) - PCB_MIN_PINORVIACOPPER;

				pcb_old_via_new(yyData, -1, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU((yyvsp[-3].measure)) + 2*PCB_MASKFRAME, hole, (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2255 "parse_y.c" /* yacc.c:1652  */
    break;

  case 71:
#line 591 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_rat_new(yyData, -1, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-4].measure)), NU ((yyvsp[-3].measure)), (yyvsp[-5].integer), (yyvsp[-2].integer),
					conf_core.appearance.rat_thickness, (yyvsp[-1].flagtype), NULL, NULL);
			}
#line 2264 "parse_y.c" /* yacc.c:1652  */
    break;

  case 72:
#line 596 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_rat_new(yyData, -1, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-5].integer), (yyvsp[-2].integer),
					conf_core.appearance.rat_thickness, pcb_flag_old((yyvsp[-1].integer)), NULL, NULL);
			}
#line 2273 "parse_y.c" /* yacc.c:1652  */
    break;

  case 73:
#line 605 "parse_y.y" /* yacc.c:1652  */
    {
				if ((yyvsp[-4].integer) <= 0 || (yyvsp[-4].integer) > PCB_MAX_LAYER)
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
				Layer->parent.data = yyData;
				Layer->parent_type = PCB_PARENT_DATA;
				Layer->type = PCB_OBJ_LAYER;

					/* memory for name is already allocated */
				if (Layer->name != NULL)
					free((char*)Layer->name);
				Layer->name = (yyvsp[-3].string);   /* shouldn't this be strdup()'ed ? */
				LayerFlag[(yyvsp[-4].integer)-1] = rnd_true;
				if (yyData->LayerN < (yyvsp[-4].integer))
				  yyData->LayerN = (yyvsp[-4].integer);
				/* In theory we could process $5 (layer type) here but this field is
				   broken (at least up to 4.3.0): it's always copper or silk; outline
				   is copper, there's no way in the GUI to create paste or mask. So
				   this field is only noise, it is safer to ignore it than to confuse
				   outline to copper. */
				if ((yyvsp[-2].string) != NULL)
					free((yyvsp[-2].string));
			}
#line 2309 "parse_y.c" /* yacc.c:1652  */
    break;

  case 85:
#line 658 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_poly_new_from_rectangle(Layer,
					OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-5].measure)) + OU ((yyvsp[-3].measure)), OU ((yyvsp[-4].measure)) + OU ((yyvsp[-2].measure)), 0, pcb_flag_old((yyvsp[-1].integer)));
			}
#line 2318 "parse_y.c" /* yacc.c:1652  */
    break;

  case 89:
#line 665 "parse_y.y" /* yacc.c:1652  */
    { attr_list = & Layer->Attributes; }
#line 2324 "parse_y.c" /* yacc.c:1652  */
    break;

  case 92:
#line 671 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_line_new(Layer, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)),
				                            NU ((yyvsp[-3].measure)), NU ((yyvsp[-2].measure)), (yyvsp[-1].flagtype));
			}
#line 2333 "parse_y.c" /* yacc.c:1652  */
    break;

  case 93:
#line 680 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_line_new(Layer, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)),
						     OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)), pcb_flag_old((yyvsp[-1].integer)));
			}
#line 2342 "parse_y.c" /* yacc.c:1652  */
    break;

  case 94:
#line 689 "parse_y.y" /* yacc.c:1652  */
    {
				/* eliminate old-style rat-lines */
			if ((IV ((yyvsp[-1].measure)) & PCB_FLAG_RAT) == 0)
				pcb_line_new(Layer, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)),
					200*PCB_GROUNDPLANEFRAME, pcb_flag_old(IV ((yyvsp[-1].measure))));
			}
#line 2353 "parse_y.c" /* yacc.c:1652  */
    break;

  case 95:
#line 700 "parse_y.y" /* yacc.c:1652  */
    {
			  pcb_arc_new(Layer, NU ((yyvsp[-9].measure)), NU ((yyvsp[-8].measure)), NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), (yyvsp[-3].number), (yyvsp[-2].number),
			                             NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-1].flagtype), rnd_true);
			}
#line 2362 "parse_y.c" /* yacc.c:1652  */
    break;

  case 96:
#line 709 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_arc_new(Layer, OU ((yyvsp[-9].measure)), OU ((yyvsp[-8].measure)), OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), (yyvsp[-3].number), (yyvsp[-2].number),
						    OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), pcb_flag_old((yyvsp[-1].integer)), rnd_true);
			}
#line 2371 "parse_y.c" /* yacc.c:1652  */
    break;

  case 97:
#line 718 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_arc_new(Layer, OU ((yyvsp[-8].measure)), OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-6].measure)), IV ((yyvsp[-3].measure)), (yyvsp[-2].number),
					OU ((yyvsp[-4].measure)), 200*PCB_GROUNDPLANEFRAME, pcb_flag_old((yyvsp[-1].integer)), rnd_true);
			}
#line 2380 "parse_y.c" /* yacc.c:1652  */
    break;

  case 98:
#line 727 "parse_y.y" /* yacc.c:1652  */
    {
					/* use a default scale of 100% */
				pcb_text_new(Layer,yyRndFont,OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].number) * 90.0, 100, 0, (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2390 "parse_y.c" /* yacc.c:1652  */
    break;

  case 99:
#line 737 "parse_y.y" /* yacc.c:1652  */
    {
				if ((yyvsp[-1].integer) & PCB_FLAG_ONSILK)
				{
					pcb_layer_t *lay = &yyData->Layer[yyData->LayerN +
						(((yyvsp[-1].integer) & PCB_FLAG_ONSOLDER) ? PCB_SOLDER_SIDE : PCB_COMPONENT_SIDE) - 2];

					pcb_text_new(lay ,yyRndFont, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), (yyvsp[-4].number) * 90.0, (yyvsp[-3].number), 0, (yyvsp[-2].string),
						      pcb_flag_old((yyvsp[-1].integer)));
				}
				else
					pcb_text_new(Layer, yyRndFont, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), (yyvsp[-4].number) * 90.0, (yyvsp[-3].number), 0, (yyvsp[-2].string),
						      pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2409 "parse_y.c" /* yacc.c:1652  */
    break;

  case 100:
#line 755 "parse_y.y" /* yacc.c:1652  */
    {
				if ((yyvsp[-2].string) == NULL) {
					rnd_message(RND_MSG_ERROR, "Empty string in text object - not loading this text object to avoid invisible objects\n");
				}
				else {
				/* FIXME: shouldn't know about .f */
				/* I don't think this matters because anything with hi_format
				 * will have the silk on its own layer in the file rather
				 * than using the PCB_FLAG_ONSILK and having it in a copper layer.
				 * Thus there is no need for anything besides the 'else'
				 * part of this code.
				 */
				if ((yyvsp[-1].flagtype).f & PCB_FLAG_ONSILK)
				{
					pcb_layer_t *lay = &yyData->Layer[yyData->LayerN +
						(((yyvsp[-1].flagtype).f & PCB_FLAG_ONSOLDER) ? PCB_SOLDER_SIDE : PCB_COMPONENT_SIDE) - 2];

					pcb_text_new(lay, yyRndFont, NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), (yyvsp[-4].number) * 90.0, (yyvsp[-3].number), 0, (yyvsp[-2].string), (yyvsp[-1].flagtype));
				}
				else
					pcb_text_new(Layer, yyRndFont, NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), (yyvsp[-4].number) * 90.0, (yyvsp[-3].number), 0, (yyvsp[-2].string), (yyvsp[-1].flagtype));
				free ((yyvsp[-2].string));
			}
			}
#line 2438 "parse_y.c" /* yacc.c:1652  */
    break;

  case 101:
#line 784 "parse_y.y" /* yacc.c:1652  */
    {
				Polygon = pcb_poly_new(Layer, 0, (yyvsp[-2].flagtype));
			}
#line 2446 "parse_y.c" /* yacc.c:1652  */
    break;

  case 102:
#line 789 "parse_y.y" /* yacc.c:1652  */
    {
				rnd_cardinal_t contour, contour_start, contour_end;
				rnd_bool bad_contour_found = rnd_false;
				/* ignore junk */
				for (contour = 0; contour <= Polygon->HoleIndexN; contour++)
				  {
				    contour_start = (contour == 0) ?
						      0 : Polygon->HoleIndex[contour - 1];
				    contour_end = (contour == Polygon->HoleIndexN) ?
						 Polygon->PointN :
						 Polygon->HoleIndex[contour];
				    if (contour_end - contour_start < 3)
				      bad_contour_found = rnd_true;
				  }

				if (bad_contour_found)
				  {
				    rnd_message(RND_MSG_WARNING, "WARNING parsing file '%s'\n"
					    "    line:        %i\n"
					    "    description: 'ignored polygon (< 3 points in a contour)'\n",
					    yyfilename, pcb_lineno);
				    pcb_destroy_object(yyData, PCB_OBJ_POLY, Layer, Polygon, Polygon);
				  }
				else
				  {
				    pcb_poly_bbox(Polygon);
				    if (!Layer->polygon_tree)
				      rnd_rtree_init(Layer->polygon_tree = malloc(sizeof(rnd_rtree_t)));
				    rnd_rtree_insert(Layer->polygon_tree, Polygon, (rnd_rtree_box_t *)Polygon);
				  }
			}
#line 2482 "parse_y.c" /* yacc.c:1652  */
    break;

  case 105:
#line 829 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_poly_hole_new(Polygon);
			}
#line 2490 "parse_y.c" /* yacc.c:1652  */
    break;

  case 109:
#line 843 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_poly_point_new(Polygon, OU ((yyvsp[-2].measure)), OU ((yyvsp[-1].measure)));
			}
#line 2498 "parse_y.c" /* yacc.c:1652  */
    break;

  case 110:
#line 847 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_poly_point_new(Polygon, NU ((yyvsp[-2].measure)), NU ((yyvsp[-1].measure)));
			}
#line 2506 "parse_y.c" /* yacc.c:1652  */
    break;

  case 116:
#line 865 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc = io_pcb_element_new(yyData, yysubc, yyRndFont, pcb_no_flags(),
					(yyvsp[-6].string), (yyvsp[-5].string), NULL, OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-2].integer), 100, pcb_no_flags(), rnd_false);
				free ((yyvsp[-6].string));
				free ((yyvsp[-5].string));
				pin_num = 1;
			}
#line 2518 "parse_y.c" /* yacc.c:1652  */
    break;

  case 117:
#line 873 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_fin(yyData);
			}
#line 2526 "parse_y.c" /* yacc.c:1652  */
    break;

  case 118:
#line 883 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc = io_pcb_element_new(yyData, yysubc, yyRndFont, pcb_flag_old((yyvsp[-9].integer)),
					(yyvsp[-8].string), (yyvsp[-7].string), NULL, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), IV ((yyvsp[-4].measure)), IV ((yyvsp[-3].measure)), pcb_flag_old((yyvsp[-2].integer)), rnd_false);
				free ((yyvsp[-8].string));
				free ((yyvsp[-7].string));
				pin_num = 1;
			}
#line 2538 "parse_y.c" /* yacc.c:1652  */
    break;

  case 119:
#line 891 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_fin(yyData);
			}
#line 2546 "parse_y.c" /* yacc.c:1652  */
    break;

  case 120:
#line 901 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc = io_pcb_element_new(yyData, yysubc, yyRndFont, pcb_flag_old((yyvsp[-10].integer)),
					(yyvsp[-9].string), (yyvsp[-8].string), (yyvsp[-7].string), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), IV ((yyvsp[-4].measure)), IV ((yyvsp[-3].measure)), pcb_flag_old((yyvsp[-2].integer)), rnd_false);
				free ((yyvsp[-9].string));
				free ((yyvsp[-8].string));
				free ((yyvsp[-7].string));
				pin_num = 1;
			}
#line 2559 "parse_y.c" /* yacc.c:1652  */
    break;

  case 121:
#line 910 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_fin(yyData);
			}
#line 2567 "parse_y.c" /* yacc.c:1652  */
    break;

  case 122:
#line 921 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc = io_pcb_element_new(yyData, yysubc, yyRndFont, pcb_flag_old((yyvsp[-12].integer)),
					(yyvsp[-11].string), (yyvsp[-10].string), (yyvsp[-9].string), OU ((yyvsp[-8].measure)) + OU ((yyvsp[-6].measure)), OU ((yyvsp[-7].measure)) + OU ((yyvsp[-5].measure)),
					(yyvsp[-4].number), (yyvsp[-3].number), pcb_flag_old((yyvsp[-2].integer)), rnd_false);
				yysubc_ox = OU ((yyvsp[-8].measure));
				yysubc_oy = OU ((yyvsp[-7].measure));
				free ((yyvsp[-11].string));
				free ((yyvsp[-10].string));
				free ((yyvsp[-9].string));
			}
#line 2582 "parse_y.c" /* yacc.c:1652  */
    break;

  case 123:
#line 932 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_fin(yyData);
			}
#line 2590 "parse_y.c" /* yacc.c:1652  */
    break;

  case 124:
#line 943 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc = io_pcb_element_new(yyData, yysubc, yyRndFont, (yyvsp[-12].flagtype),
					(yyvsp[-11].string), (yyvsp[-10].string), (yyvsp[-9].string), NU ((yyvsp[-8].measure)) + NU ((yyvsp[-6].measure)), NU ((yyvsp[-7].measure)) + NU ((yyvsp[-5].measure)),
					(yyvsp[-4].number), (yyvsp[-3].number), (yyvsp[-2].flagtype), rnd_false);
				yysubc_ox = NU ((yyvsp[-8].measure));
				yysubc_oy = NU ((yyvsp[-7].measure));
				free ((yyvsp[-11].string));
				free ((yyvsp[-10].string));
				free ((yyvsp[-9].string));
			}
#line 2605 "parse_y.c" /* yacc.c:1652  */
    break;

  case 125:
#line 954 "parse_y.y" /* yacc.c:1652  */
    {
				if (pcb_subc_is_empty(yysubc)) {
					pcb_subc_free(yysubc);
					yysubc = NULL;
				}
				else {
					io_pcb_element_fin(yyData);
				}
			}
#line 2619 "parse_y.c" /* yacc.c:1652  */
    break;

  case 133:
#line 978 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_line_new(yysubc, NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), NU ((yyvsp[-3].measure)), NU ((yyvsp[-2].measure)), NU ((yyvsp[-1].measure)));
			}
#line 2627 "parse_y.c" /* yacc.c:1652  */
    break;

  case 134:
#line 983 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_line_new(yysubc, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)), OU ((yyvsp[-1].measure)));
			}
#line 2635 "parse_y.c" /* yacc.c:1652  */
    break;

  case 135:
#line 988 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_arc_new(yysubc, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), NU ((yyvsp[-1].measure)));
			}
#line 2643 "parse_y.c" /* yacc.c:1652  */
    break;

  case 136:
#line 993 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_arc_new(yysubc, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), OU ((yyvsp[-1].measure)));
			}
#line 2651 "parse_y.c" /* yacc.c:1652  */
    break;

  case 137:
#line 998 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc_ox = NU ((yyvsp[-2].measure));
				yysubc_oy = NU ((yyvsp[-1].measure));
			}
#line 2660 "parse_y.c" /* yacc.c:1652  */
    break;

  case 138:
#line 1003 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc_ox = OU ((yyvsp[-2].measure));
				yysubc_oy = OU ((yyvsp[-1].measure));
			}
#line 2669 "parse_y.c" /* yacc.c:1652  */
    break;

  case 139:
#line 1007 "parse_y.y" /* yacc.c:1652  */
    { attr_list = & yysubc->Attributes; }
#line 2675 "parse_y.c" /* yacc.c:1652  */
    break;

  case 148:
#line 1023 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_line_new(yysubc, NU ((yyvsp[-5].measure)) + yysubc_ox,
					NU ((yyvsp[-4].measure)) + yysubc_oy, NU ((yyvsp[-3].measure)) + yysubc_ox,
					NU ((yyvsp[-2].measure)) + yysubc_oy, NU ((yyvsp[-1].measure)));
			}
#line 2685 "parse_y.c" /* yacc.c:1652  */
    break;

  case 149:
#line 1029 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_line_new(yysubc, OU ((yyvsp[-5].measure)) + yysubc_ox,
					OU ((yyvsp[-4].measure)) + yysubc_oy, OU ((yyvsp[-3].measure)) + yysubc_ox,
					OU ((yyvsp[-2].measure)) + yysubc_oy, OU ((yyvsp[-1].measure)));
			}
#line 2695 "parse_y.c" /* yacc.c:1652  */
    break;

  case 150:
#line 1036 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_arc_new(yysubc, NU ((yyvsp[-7].measure)) + yysubc_ox,
					NU ((yyvsp[-6].measure)) + yysubc_oy, NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), NU ((yyvsp[-1].measure)));
			}
#line 2704 "parse_y.c" /* yacc.c:1652  */
    break;

  case 151:
#line 1041 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_arc_new(yysubc, OU ((yyvsp[-7].measure)) + yysubc_ox,
					OU ((yyvsp[-6].measure)) + yysubc_oy, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), OU ((yyvsp[-1].measure)));
			}
#line 2713 "parse_y.c" /* yacc.c:1652  */
    break;

  case 152:
#line 1045 "parse_y.y" /* yacc.c:1652  */
    { attr_list = & yysubc->Attributes; }
#line 2719 "parse_y.c" /* yacc.c:1652  */
    break;

  case 154:
#line 1052 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_pstk_t *pin = io_pcb_element_pin_new(yysubc, NU ((yyvsp[-9].measure)) + yysubc_ox,
					NU ((yyvsp[-8].measure)) + yysubc_oy, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-3].string),
					(yyvsp[-2].string), (yyvsp[-1].flagtype));
				pcb_attrib_compat_set_intconn(&pin->Attributes, yy_intconn);
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2732 "parse_y.c" /* yacc.c:1652  */
    break;

  case 155:
#line 1065 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_pin_new(yysubc, OU ((yyvsp[-9].measure)) + yysubc_ox,
					OU ((yyvsp[-8].measure)) + yysubc_oy, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].string),
					(yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2744 "parse_y.c" /* yacc.c:1652  */
    break;

  case 156:
#line 1077 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_pin_new(yysubc, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU ((yyvsp[-5].measure)) + 2*PCB_MASKFRAME, OU ((yyvsp[-4].measure)), (yyvsp[-3].string), (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2755 "parse_y.c" /* yacc.c:1652  */
    break;

  case 157:
#line 1088 "parse_y.y" /* yacc.c:1652  */
    {
				char	p_number[8];

				sprintf(p_number, "%d", pin_num++);
				io_pcb_element_pin_new(yysubc, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU ((yyvsp[-4].measure)) + 2*PCB_MASKFRAME, OU ((yyvsp[-3].measure)), (yyvsp[-2].string), p_number, pcb_flag_old((yyvsp[-1].integer)));

				free ((yyvsp[-2].string));
			}
#line 2769 "parse_y.c" /* yacc.c:1652  */
    break;

  case 158:
#line 1104 "parse_y.y" /* yacc.c:1652  */
    {
				rnd_coord_t	hole = OU ((yyvsp[-3].measure)) * PCB_DEFAULT_DRILLINGHOLE;
				char	p_number[8];

					/* make sure that there's enough copper left */
				if (OU ((yyvsp[-3].measure)) - hole < PCB_MIN_PINORVIACOPPER &&
					OU ((yyvsp[-3].measure)) > PCB_MIN_PINORVIACOPPER)
					hole = OU ((yyvsp[-3].measure)) - PCB_MIN_PINORVIACOPPER;

				sprintf(p_number, "%d", pin_num++);
				io_pcb_element_pin_new(yysubc, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU ((yyvsp[-3].measure)) + 2*PCB_MASKFRAME, hole, (yyvsp[-2].string), p_number, pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2788 "parse_y.c" /* yacc.c:1652  */
    break;

  case 159:
#line 1123 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_pstk_t *pad = io_pcb_element_pad_new(yysubc, NU ((yyvsp[-10].measure)) + yysubc_ox,
					NU ((yyvsp[-9].measure)) + yysubc_oy,
					NU ((yyvsp[-8].measure)) + yysubc_ox,
					NU ((yyvsp[-7].measure)) + yysubc_oy, NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)),
					(yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].flagtype));
				pcb_attrib_compat_set_intconn(&pad->Attributes, yy_intconn);
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2803 "parse_y.c" /* yacc.c:1652  */
    break;

  case 160:
#line 1138 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_pad_new(yysubc,OU ((yyvsp[-10].measure)) + yysubc_ox,
					OU ((yyvsp[-9].measure)) + yysubc_oy, OU ((yyvsp[-8].measure)) + yysubc_ox,
					OU ((yyvsp[-7].measure)) + yysubc_oy, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)),
					(yyvsp[-3].string), (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2816 "parse_y.c" /* yacc.c:1652  */
    break;

  case 161:
#line 1151 "parse_y.y" /* yacc.c:1652  */
    {
				rnd_coord_t cx = OU((yyvsp[-9].measure)), cy = OU((yyvsp[-8].measure)), sx = OU((yyvsp[-7].measure)), sy = OU((yyvsp[-6].measure));
				rnd_coord_t x1, y1, x2, y2;
				double ox, oy, thick;

				thick = (sx > sy) ? sx : sy;
				ox = (sx > sy) ? ((sx - sy) / 2.0) : 0;
				oy = (sx > sy) ? 0 : ((sx - sy) / 2.0);

				x1 = rnd_round(cx - ox); y1 = rnd_round(cy - oy);
				x2 = rnd_round(cx + ox); y2 = rnd_round(cy + oy);

				io_pcb_element_pad_new(yysubc,
					x1 + yysubc_ox, y1 + yysubc_oy,
					x2 + yysubc_ox, y2 + yysubc_oy,
					OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)),
					(yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].flagtype));
				free ((yyvsp[-2].string));
			}
#line 2840 "parse_y.c" /* yacc.c:1652  */
    break;

  case 162:
#line 1175 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_pad_new(yysubc,OU ((yyvsp[-8].measure)),OU ((yyvsp[-7].measure)),OU ((yyvsp[-6].measure)),OU ((yyvsp[-5].measure)),OU ((yyvsp[-4].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU ((yyvsp[-4].measure)) + 2*PCB_MASKFRAME, (yyvsp[-3].string), (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2851 "parse_y.c" /* yacc.c:1652  */
    break;

  case 163:
#line 1186 "parse_y.y" /* yacc.c:1652  */
    {
				char		p_number[8];

				sprintf(p_number, "%d", pin_num++);
				io_pcb_element_pad_new(yysubc,OU ((yyvsp[-7].measure)),OU ((yyvsp[-6].measure)),OU ((yyvsp[-5].measure)),OU ((yyvsp[-4].measure)),OU ((yyvsp[-3].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU ((yyvsp[-3].measure)) + 2*PCB_MASKFRAME, (yyvsp[-2].string),p_number, pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2864 "parse_y.c" /* yacc.c:1652  */
    break;

  case 164:
#line 1196 "parse_y.y" /* yacc.c:1652  */
    { (yyval.flagtype) = pcb_flag_old((yyvsp[0].integer)); }
#line 2870 "parse_y.c" /* yacc.c:1652  */
    break;

  case 165:
#line 1197 "parse_y.y" /* yacc.c:1652  */
    { (yyval.flagtype) = pcb_strflg_s2f((yyvsp[0].string), yyerror, &yy_intconn, 1); free((yyvsp[0].string)); }
#line 2876 "parse_y.c" /* yacc.c:1652  */
    break;

  case 169:
#line 1208 "parse_y.y" /* yacc.c:1652  */
    {
				if ((yyvsp[-3].integer) <= 0 || (yyvsp[-3].integer) > PCB_MAX_FONTPOSITION)
				{
					yyerror("fontposition out of range");
					YYABORT;
				}
				Glyph = &yyRndFont->glyph[(yyvsp[-3].integer)];
				if (Glyph->valid)
				{
					yyerror("symbol ID used twice");
					YYABORT;
				}
				Glyph->valid = rnd_true;
				Glyph->xdelta = NU ((yyvsp[-2].measure));
			}
#line 2896 "parse_y.c" /* yacc.c:1652  */
    break;

  case 170:
#line 1224 "parse_y.y" /* yacc.c:1652  */
    {
				if ((yyvsp[-3].integer) <= 0 || (yyvsp[-3].integer) > PCB_MAX_FONTPOSITION)
				{
					yyerror("fontposition out of range");
					YYABORT;
				}
				Glyph = &yyRndFont->glyph[(yyvsp[-3].integer)];
				if (Glyph->valid)
				{
					yyerror("symbol ID used twice");
					YYABORT;
				}
				Glyph->valid = rnd_true;
				Glyph->xdelta = OU ((yyvsp[-2].measure));
			}
#line 2916 "parse_y.c" /* yacc.c:1652  */
    break;

  case 176:
#line 1255 "parse_y.y" /* yacc.c:1652  */
    {
				rnd_font_new_line_in_glyph(Glyph, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)), OU ((yyvsp[-1].measure)));
			}
#line 2924 "parse_y.c" /* yacc.c:1652  */
    break;

  case 177:
#line 1262 "parse_y.y" /* yacc.c:1652  */
    {
				rnd_font_new_line_in_glyph(Glyph, NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), NU ((yyvsp[-3].measure)), NU ((yyvsp[-2].measure)), NU ((yyvsp[-1].measure)));
			}
#line 2932 "parse_y.c" /* yacc.c:1652  */
    break;

  case 185:
#line 1289 "parse_y.y" /* yacc.c:1652  */
    {
				currnet = pcb_net_get(yyPCB, &yyPCB->netlist[PCB_NETLIST_INPUT], (yyvsp[-3].string), PCB_NETA_ALLOC);
				if (((yyvsp[-2].string) != NULL) && (*(yyvsp[-2].string) != '\0'))
					pcb_attribute_put(&currnet->Attributes, "style", (yyvsp[-2].string));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2944 "parse_y.c" /* yacc.c:1652  */
    break;

  case 191:
#line 1311 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_net_term_get_by_pinname(currnet, (yyvsp[-1].string), 1);
				free ((yyvsp[-1].string));
			}
#line 2953 "parse_y.c" /* yacc.c:1652  */
    break;

  case 199:
#line 1358 "parse_y.y" /* yacc.c:1652  */
    { pcb_ratspatch_append(yyPCB, RATP_ADD_CONN, (yyvsp[-2].string), (yyvsp[-1].string), NULL, 0); free((yyvsp[-2].string)); free((yyvsp[-1].string)); }
#line 2959 "parse_y.c" /* yacc.c:1652  */
    break;

  case 200:
#line 1359 "parse_y.y" /* yacc.c:1652  */
    { pcb_ratspatch_append(yyPCB, RATP_DEL_CONN, (yyvsp[-2].string), (yyvsp[-1].string), NULL, 0); free((yyvsp[-2].string)); free((yyvsp[-1].string)); }
#line 2965 "parse_y.c" /* yacc.c:1652  */
    break;

  case 201:
#line 1360 "parse_y.y" /* yacc.c:1652  */
    { pcb_ratspatch_append(yyPCB, RATP_CHANGE_COMP_ATTRIB, (yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].string), 0); free((yyvsp[-3].string)); free((yyvsp[-2].string)); free((yyvsp[-1].string)); }
#line 2971 "parse_y.c" /* yacc.c:1652  */
    break;

  case 202:
#line 1365 "parse_y.y" /* yacc.c:1652  */
    {
				char *old_val, *key = (yyvsp[-2].string), *val = (yyvsp[-1].string) ? (yyvsp[-1].string) : (char *)"";
				old_val = pcb_attribute_get(attr_list, key);
				if (old_val != NULL)
					rnd_message(RND_MSG_ERROR, "mutliple values for attribute %s: '%s' and '%s' - ignoring '%s'\n", key, old_val, val, val);
				else
					pcb_attribute_put(attr_list, key, val);
				free(key);
				if ((yyvsp[-1].string) != NULL) free(val);
			}
#line 2986 "parse_y.c" /* yacc.c:1652  */
    break;

  case 203:
#line 1377 "parse_y.y" /* yacc.c:1652  */
    { (yyval.string) = (yyvsp[0].string); }
#line 2992 "parse_y.c" /* yacc.c:1652  */
    break;

  case 204:
#line 1378 "parse_y.y" /* yacc.c:1652  */
    { (yyval.string) = 0; }
#line 2998 "parse_y.c" /* yacc.c:1652  */
    break;

  case 205:
#line 1382 "parse_y.y" /* yacc.c:1652  */
    { (yyval.number) = (yyvsp[0].number); }
#line 3004 "parse_y.c" /* yacc.c:1652  */
    break;

  case 206:
#line 1383 "parse_y.y" /* yacc.c:1652  */
    { (yyval.number) = (yyvsp[0].integer); }
#line 3010 "parse_y.c" /* yacc.c:1652  */
    break;

  case 207:
#line 1388 "parse_y.y" /* yacc.c:1652  */
    { do_measure(&(yyval.measure), (yyvsp[0].number), RND_MIL_TO_COORD ((yyvsp[0].number)) / 100.0, 0); }
#line 3016 "parse_y.c" /* yacc.c:1652  */
    break;

  case 208:
#line 1389 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MIL_TO_COORD ((yyvsp[-1].number)) / 100000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3022 "parse_y.c" /* yacc.c:1652  */
    break;

  case 209:
#line 1390 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MIL_TO_COORD ((yyvsp[-1].number)) / 100.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3028 "parse_y.c" /* yacc.c:1652  */
    break;

  case 210:
#line 1391 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MIL_TO_COORD ((yyvsp[-1].number))); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3034 "parse_y.c" /* yacc.c:1652  */
    break;

  case 211:
#line 1392 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_INCH_TO_COORD ((yyvsp[-1].number))); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3040 "parse_y.c" /* yacc.c:1652  */
    break;

  case 212:
#line 1393 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MM_TO_COORD ((yyvsp[-1].number)) / 1000000.0); pcb_io_pcb_usty_seen |= PCB_USTY_NANOMETER; }
#line 3046 "parse_y.c" /* yacc.c:1652  */
    break;

  case 213:
#line 1394 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MM_TO_COORD ((yyvsp[-1].number)) / 1000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3052 "parse_y.c" /* yacc.c:1652  */
    break;

  case 214:
#line 1395 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MM_TO_COORD ((yyvsp[-1].number))); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3058 "parse_y.c" /* yacc.c:1652  */
    break;

  case 215:
#line 1396 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MM_TO_COORD ((yyvsp[-1].number)) * 1000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3064 "parse_y.c" /* yacc.c:1652  */
    break;

  case 216:
#line 1397 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MM_TO_COORD ((yyvsp[-1].number)) * 1000000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3070 "parse_y.c" /* yacc.c:1652  */
    break;


#line 3074 "parse_y.c" /* yacc.c:1652  */
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
#line 1400 "parse_y.y" /* yacc.c:1918  */


/* ---------------------------------------------------------------------------
 * error routine called by parser library
 */
int yyerror(const char * s)
{
	rnd_message(RND_MSG_ERROR, "ERROR parsing file '%s'\n"
		"    line:        %i\n"
		"    description: '%s'\n",
		yyfilename, pcb_lineno, s);
	return(0);
}

int pcb_wrap()
{
  return 1;
}

static int
check_file_version (int ver)
{
  if ( ver > PCB_FILE_VERSION ) {
    rnd_message(RND_MSG_ERROR, "ERROR:  The file you are attempting to load is in a format\n"
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
do_measure (PLMeasure *m, rnd_coord_t i, double d, int u)
{
  m->ival = i;
  m->bval = rnd_round(d);
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

static rnd_coord_t
old_units (PLMeasure m)
{
  if (m.has_units)
    return m.bval;
  if (m.ival != 0)
    pcb_io_pcb_usty_seen |= PCB_USTY_CMIL; /* ... because we can't save in mil */
  return rnd_round(RND_MIL_TO_COORD (m.ival));
}

static rnd_coord_t
new_units (PLMeasure m)
{
  if (m.has_units)
    return m.bval;
  if (m.dval != 0)
    pcb_io_pcb_usty_seen |= PCB_USTY_CMIL;
  /* if there's no unit m.dval already contains the converted value */
  return rnd_round(m.dval);
}

/* This converts old flag bits (from saved PCB files) to new format.  */
static pcb_flag_t pcb_flag_old(unsigned int flags)
{
	pcb_flag_t rv;
	int i, f;
	memset(&rv, 0, sizeof(rv));
	/* If we move flag bits around, this is where we map old bits to them.  */
	rv.f = flags & 0xffff;
	f = 0x10000;
	for (i = 0; i < 8; i++) {
		/* use the closest thing to the old thermal style */
		if (flags & f)
			rv.t[i / 2] |= (1 << (4 * (i % 2)));
		f <<= 1;
	}
	return rv;
}

/* load a board metadata into conf_core */
static void load_meta_coord(const char *path, rnd_coord_t crd)
{
	if (yy_settings_dest != RND_CFR_invalid) {
		char tmp[128];
		rnd_sprintf(tmp, "%$mm", crd);
		rnd_conf_set(yy_settings_dest, path, -1, tmp, RND_POL_OVERWRITE);
	}
}

static void load_meta_float(const char *path, double val)
{
	if (yy_settings_dest != RND_CFR_invalid) {
		char tmp[128];
		rnd_sprintf(tmp, "%f", val);
		rnd_conf_set(yy_settings_dest, path, -1, tmp, RND_POL_OVERWRITE);
	}
}
