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
#define YYLAST   619

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  55
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  114
/* YYNRULES -- Number of rules.  */
#define YYNRULES  218
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  654

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
       0,   154,   154,   155,   156,   157,   161,   161,   236,   236,
     261,   261,   280,   281,   286,   286,   306,   308,   318,   325,
     332,   342,   343,   344,   347,   355,   370,   385,   389,   393,
     396,   398,   405,   407,   415,   417,   418,   419,   423,   433,
     444,   456,   460,   465,   469,   481,   485,   494,   503,   507,
     508,   512,   513,   517,   518,   518,   519,   520,   522,   522,
     529,   533,   534,   535,   536,   537,   538,   543,   553,   563,
     574,   584,   594,   610,   615,   625,   624,   660,   661,   665,
     666,   670,   671,   672,   673,   674,   675,   677,   682,   683,
     684,   685,   685,   686,   690,   699,   708,   719,   728,   737,
     746,   756,   774,   804,   803,   842,   844,   849,   848,   855,
     857,   862,   866,   873,   874,   875,   876,   877,   885,   884,
     903,   902,   921,   920,   941,   939,   963,   961,   986,   987,
     991,   992,   993,   994,   995,   997,  1002,  1007,  1012,  1017,
    1022,  1027,  1027,  1031,  1032,  1036,  1037,  1038,  1039,  1040,
    1042,  1048,  1055,  1060,  1065,  1065,  1071,  1084,  1096,  1107,
    1123,  1142,  1157,  1170,  1194,  1205,  1216,  1217,  1221,  1222,
    1225,  1227,  1243,  1262,  1263,  1266,  1268,  1269,  1274,  1281,
    1287,  1288,  1292,  1297,  1298,  1302,  1303,  1309,  1308,  1320,
    1321,  1325,  1326,  1330,  1347,  1348,  1352,  1357,  1358,  1362,
    1363,  1378,  1379,  1380,  1384,  1397,  1398,  1402,  1403,  1408,
    1409,  1410,  1411,  1412,  1413,  1414,  1415,  1416,  1417
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
  "pcbdefinition", "$@5", "$@6", "via", "via_ehi_format", "via_hi_format",
  "via_2.0_format", "via_1.7_format", "via_newformat", "via_oldformat",
  "rats", "layer", "$@7", "layerdata", "layerdefinitions",
  "layerdefinition", "$@8", "line_hi_format", "line_1.7_format",
  "line_oldformat", "arc_hi_format", "arc_1.7_format", "arc_oldformat",
  "text_oldformat", "text_newformat", "text_hi_format", "polygon_format",
  "$@9", "polygonholes", "polygonhole", "$@10", "polygonpoints",
  "polygonpoint", "element", "element_oldformat", "$@11",
  "element_1.3.4_format", "$@12", "element_newformat", "$@13",
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

#define YYPACT_NINF -429

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-429)))

#define YYTABLE_NINF -92

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     151,  -429,    16,  -429,    12,    21,  -429,    23,  -429,    71,
    -429,    43,    97,    48,  -429,  -429,  -429,  -429,  -429,  -429,
    -429,    60,    55,    66,  -429,   169,  -429,    88,    21,  -429,
    -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,    83,    71,
    -429,  -429,   110,    92,   124,   183,   203,   144,    22,    22,
      22,    22,  -429,    96,  -429,  -429,    73,    73,  -429,   -13,
     105,   157,   158,   174,   143,  -429,  -429,  -429,  -429,  -429,
     165,   168,   175,   187,  -429,  -429,   211,    22,    22,    22,
      22,   188,  -429,  -429,    22,    22,   220,  -429,  -429,  -429,
    -429,    22,     3,    22,    22,   224,   163,   196,   207,    22,
     208,  -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,
      22,    22,   213,   231,   237,   176,   228,    22,    22,    22,
    -429,    22,    22,    22,    22,    22,   198,   253,   293,    45,
      22,  -429,   254,    22,    49,    22,    22,   258,   274,   286,
      22,    22,   294,   316,    22,    22,    22,    22,    22,   296,
     347,    22,    22,    22,   367,   338,    22,   388,   266,    22,
      22,  -429,  -429,  -429,    22,    22,  -429,  -429,   390,     4,
      22,    22,   345,    22,   350,   377,  -429,  -429,  -429,    22,
      22,    22,   348,  -429,    22,   349,   402,   275,   403,   404,
      22,    22,   357,   356,  -429,   359,   358,  -429,   361,    22,
     362,   387,    22,    22,    22,   363,   170,   287,  -429,   364,
     413,   414,   183,   415,    22,    22,  -429,  -429,  -429,  -429,
    -429,    22,   327,   368,   394,    22,    22,   419,  -429,   281,
     284,   371,   285,   372,   373,   170,  -429,    88,  -429,  -429,
    -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,   422,   183,
    -429,   374,   425,   378,   380,   383,   386,    22,   389,   391,
     430,   301,   409,    22,    79,   392,   172,    22,    22,    22,
      22,    22,    22,    22,   183,  -429,  -429,  -429,   436,   395,
    -429,   397,  -429,  -429,  -429,  -429,     8,  -429,  -429,   398,
     438,   442,   195,  -429,    22,   399,    22,   396,   302,   410,
     411,   305,   306,   102,  -429,    88,  -429,  -429,  -429,  -429,
    -429,    22,    22,    22,    22,    22,    22,    22,   408,   183,
    -429,  -429,  -429,    11,  -429,   416,   412,   434,   183,   417,
     463,  -429,    22,    22,    22,    22,    22,    22,    22,    22,
    -429,  -429,  -429,    22,    22,    22,    22,    22,    22,    22,
     418,   423,  -429,    22,  -429,  -429,   421,   435,  -429,   424,
    -429,   426,   172,    22,    22,    22,    22,    22,    22,    22,
      22,    22,    22,    22,    22,    22,    22,   299,  -429,  -429,
     427,   428,   431,  -429,  -429,   432,   172,   433,   121,    22,
      22,    22,    22,    22,    22,   429,   443,    22,    22,    22,
      22,   473,   472,   479,   481,   309,  -429,   445,   446,  -429,
     226,  -429,  -429,    22,    22,   303,    22,    22,    22,  -429,
    -429,    22,    22,    22,    22,   447,   183,   448,   492,    22,
      22,  -429,   309,   467,   450,   214,  -429,   214,    22,    22,
     500,   499,    22,    22,    22,   183,     5,    22,    22,  -429,
     454,  -429,   453,    22,    22,   -17,  -429,   455,   464,   467,
    -429,    30,   310,   325,   326,   331,     2,  -429,    88,  -429,
    -429,  -429,  -429,  -429,   222,   457,   465,   466,   379,   515,
      22,    22,   470,   469,  -429,    22,    85,  -429,  -429,   474,
     471,   475,  -429,  -429,   518,  -429,  -429,   476,   485,   486,
     487,    30,  -429,    22,    22,    22,    22,    22,    22,    22,
      22,  -429,  -429,  -429,  -429,  -429,  -429,  -429,   488,   523,
     382,    22,    22,  -429,  -429,   183,   489,   536,  -429,  -429,
    -429,   544,   547,   548,   549,  -429,  -429,    22,    22,    22,
      22,    22,    22,    22,    22,  -429,   506,   509,   560,   513,
     512,   516,  -429,   520,   309,   521,   561,   563,   570,    22,
      22,    22,    22,    22,    22,    22,    22,  -429,  -429,   524,
    -429,  -429,  -429,  -429,   525,   527,   528,   529,   571,    22,
      22,    22,    22,    22,    22,    22,    22,  -429,  -429,  -429,
    -429,  -429,   530,    22,    22,    22,    22,    22,    22,    22,
      22,   552,  -429,   534,   533,    22,    22,    22,    22,    22,
      22,   535,   537,   552,  -429,  -429,  -429,   575,   583,   307,
      22,    22,    22,   584,  -429,  -429,   586,   587,   588,   589,
     590,   545,   546,   550,   183,   594,   183,   593,   595,  -429,
    -429,  -429,   551,   553,   554,   183,   598,  -429,  -429,  -429,
     556,   555,  -429,  -429
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     5,     0,     2,    16,     0,     3,     0,     4,     0,
       1,     0,     0,     0,     9,   113,   114,   115,   116,   117,
      60,     0,     0,     0,    11,     0,    51,     0,     0,    53,
      61,    62,    63,    64,    65,    66,    56,    57,     0,    15,
     168,   175,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    52,     0,    55,    59,     0,     0,   169,     0,
       0,     0,     0,     0,    29,    21,    22,    23,   166,   167,
       0,     0,     0,     0,   207,   208,   209,     0,     0,     0,
       0,     0,   173,   174,     0,     0,     0,   170,   176,   177,
      17,     0,     0,     0,     0,     0,    30,     0,     0,     0,
     206,   210,   211,   212,   213,   214,   215,   216,   217,   218,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      18,     0,     0,     0,     0,     0,     0,    32,     0,     0,
       0,   205,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      34,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   204,   171,   172,     0,     0,    20,    19,     0,     0,
       0,     0,     0,     0,     0,    43,    35,    36,    37,     0,
       0,     0,     0,    75,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    24,     0,     0,    31,     0,     0,
       0,    45,     0,     0,     0,     0,    78,     0,    72,     0,
       0,     0,     0,     0,     0,     0,    26,    25,    28,    27,
      33,     0,     0,     0,    48,     0,     0,     0,   118,     0,
       0,     0,     0,     0,     0,    77,    79,     0,    81,    82,
      83,    84,    85,    86,    90,    89,    88,    93,     0,     0,
      71,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    13,     0,     0,     0,   141,     0,     0,     0,
       0,     0,     0,     0,     0,    76,    80,    92,     0,     0,
      70,     0,    73,    74,   179,   178,     0,    41,    42,     0,
       0,     0,     0,    12,     0,   208,     0,     0,     0,     0,
       0,     0,     0,   141,   128,     0,   130,   131,   132,   133,
     134,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      68,    69,    38,     0,    44,     0,     0,   181,     0,     0,
       0,   120,     0,     0,     0,     0,     0,     0,     0,     0,
     119,   129,   142,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    39,     0,    47,    46,     0,   195,   180,     0,
     122,     0,   141,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   103,    67,
       0,     0,     0,     7,   194,     0,   141,     0,   141,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   109,    40,     0,     0,   126,
     141,   124,   121,     0,     0,     0,     0,     0,     0,   139,
     140,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   105,   109,   184,     0,   154,   123,   154,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    87,
       0,   100,     0,     0,     0,     0,   110,     0,     0,   183,
     185,   198,     0,     0,     0,     0,   154,   143,     0,   146,
     145,   149,   147,   148,   154,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    96,     0,     0,   102,   101,     0,
       0,     0,   104,   106,     0,   182,   186,     0,     0,     0,
       0,   197,   199,     0,     0,     0,     0,     0,     0,     0,
       0,   127,   144,   155,   125,   135,   136,   160,     0,     0,
       0,     0,     0,    94,    95,     0,   208,     0,   112,   111,
     107,     0,     0,     0,     0,   196,   200,     0,     0,     0,
       0,     0,     0,     0,     0,   159,     0,     0,     0,     0,
       0,     0,    99,     0,   109,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   158,   165,     0,
     137,   138,    97,    98,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   164,   108,   187,
     201,   202,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   190,   203,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   189,   191,   150,   151,     0,     0,     0,
       0,     0,     0,     0,   188,   192,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   152,
     153,   193,     0,     0,     0,     0,     0,   156,   157,   163,
       0,     0,   161,   162
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,   343,  -429,
    -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,
    -429,  -429,  -429,  -429,  -429,  -429,   318,  -429,   591,  -429,
    -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,
    -429,  -429,  -429,   376,  -429,  -429,  -429,  -429,  -429,  -429,
    -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,  -428,
    -429,   585,  -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,
    -429,  -429,  -340,  -298,  -429,   177,  -427,  -429,  -429,  -429,
    -429,  -429,  -429,  -429,  -429,  -429,  -429,  -429,  -195,  -429,
     573,  -429,   558,  -429,  -429,  -429,  -429,  -429,  -429,  -429,
     159,  -429,  -429,  -429,     6,  -429,  -429,  -429,  -429,   116,
    -234,  -429,   -48,   -49
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     4,     5,     6,     7,   292,     8,     9,
      12,    44,    64,    65,    66,    67,    96,   127,   150,   175,
     176,   177,   178,   201,   224,   262,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
     206,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   405,   455,   493,   554,   431,
     432,    14,    15,   266,    16,   362,    17,   386,    18,   437,
      19,   435,   303,   304,   305,   466,   467,   468,   469,   470,
     306,   307,   308,   471,   472,   473,   309,   310,    70,    39,
      40,    41,    84,    59,    88,    89,   357,   358,   458,   459,
     460,   601,   612,   613,   614,   383,   384,   500,   501,   502,
      54,   132,    76,    77
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      78,    79,    80,   277,   456,   341,    74,    75,   193,   483,
      86,    74,    75,   491,    74,    75,    10,   253,   462,    11,
     463,   464,   388,   -50,    20,    74,    75,   465,   110,   111,
     112,   113,    21,    22,    23,   115,   116,   492,    13,   512,
     -58,    87,   119,   121,   122,   123,   410,   512,    74,    75,
     130,   152,    74,    75,   279,   157,   511,   120,   194,   484,
     322,   133,   134,   352,   -54,   497,   498,   499,   140,   141,
     142,   342,   143,   144,   145,   146,   147,    82,    83,   318,
     153,   154,    74,   295,   156,   158,   159,   160,    74,   526,
     341,   164,   165,    38,    42,   168,   169,   170,   171,    45,
     172,    46,   179,   180,   181,    43,    48,   184,    49,   187,
     188,   189,   341,    47,    60,   190,   191,    50,   298,    51,
     299,   300,   195,   196,   351,   198,   574,   301,   302,    53,
     202,   203,   204,   359,    56,   207,    57,   298,   211,   299,
     300,   214,   215,    61,    63,    62,   301,   302,    73,    81,
     221,   -10,     1,   225,   226,   227,   340,    90,    -6,    -6,
     -10,   -10,   -10,    91,    92,   255,   256,    95,    -8,   -49,
      20,    97,   257,   -14,    98,   412,   263,   264,    21,    22,
      23,    99,   229,   230,   231,   232,   -58,    68,   298,    69,
     299,   300,   -10,   100,   114,   -50,    20,   301,   302,   233,
     -49,   126,   128,   -49,    21,    22,    23,    71,   286,    72,
     -54,   -91,   -58,   129,   131,   294,   296,   135,   311,   312,
     313,   314,   315,   316,   317,    93,   -50,    94,   138,   -50,
     462,   450,   463,   464,   513,   136,   -54,   323,   462,   465,
     463,   464,   298,   137,   299,   300,   328,   465,   330,   148,
     482,   301,   302,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   343,   344,   345,   346,   347,   348,   349,    74,
      75,   117,   186,   118,   353,   124,   514,   125,    74,    75,
     436,   210,   139,   363,   364,   365,   366,   367,   368,   369,
     370,   248,   149,   249,   371,   372,   373,   374,   375,   151,
     376,   377,    74,    75,   380,   403,    74,    75,   155,   440,
      74,    75,   161,   628,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   162,   402,   404,
     551,   258,   267,   259,   268,   269,   272,   270,   273,   163,
     413,   414,   415,   416,   417,   418,   166,   173,   421,   422,
     423,   424,   290,   332,   291,   333,   336,   338,   337,   339,
     429,   503,   430,   504,   438,   439,   441,   442,   443,   444,
     167,   182,   445,   446,   447,   448,   505,   507,   506,   508,
     453,   454,   509,   518,   510,   519,   547,   174,   548,   475,
     476,   183,   185,   479,   192,   480,   481,   197,   200,   485,
     486,   199,   205,   208,   489,   490,   209,   212,   213,   216,
     217,   218,   219,   220,   223,   222,   228,   251,   250,   254,
     252,   260,   261,   265,   271,   274,   278,   275,   280,   281,
     282,   -14,   521,   522,   283,   284,   289,   525,   527,   642,
     285,   644,   319,   287,   325,   288,   297,   320,   326,   331,
     650,   321,   324,   329,   537,   538,   539,   540,   541,   542,
     543,   544,   350,   334,   335,   356,   355,   361,   354,   382,
     360,   378,   549,   550,   381,   379,   385,   425,   426,   406,
     387,   419,   407,   427,   408,   409,   411,   428,   559,   560,
     561,   562,   563,   564,   565,   566,   452,   420,   433,   457,
     434,   449,   451,   461,   477,   478,   487,   488,   494,   515,
     579,   580,   581,   582,   583,   584,   585,   586,   495,   516,
     517,   520,   523,   524,   531,   529,   528,   546,   530,   532,
     593,   594,   595,   596,   597,   598,   599,   600,   533,   534,
     553,   535,   545,   552,   603,   604,   605,   606,   607,   608,
     555,   609,   610,   556,   557,   558,   617,   618,   619,   620,
     567,   621,   622,   568,   569,   570,   571,   576,   572,   577,
     629,   630,   631,   632,   573,   575,   578,   592,   587,   588,
     589,   626,   590,   591,   602,   611,   615,   616,   623,   627,
     633,   624,   634,   635,   636,   637,   638,   639,   643,   645,
     640,   646,   651,   647,   641,   293,   649,   648,   652,   653,
     327,   276,    58,    55,   474,    85,    52,   536,   496,   625
};

static const yytype_uint16 yycheck[] =
{
      49,    50,    51,   237,   432,   303,     3,     4,     4,     4,
      23,     3,     4,    30,     3,     4,     0,   212,    16,     7,
      18,    19,   362,     0,     1,     3,     4,    25,    77,    78,
      79,    80,     9,    10,    11,    84,    85,    54,    17,   466,
      17,    54,    91,    92,    93,    94,   386,   474,     3,     4,
      99,     6,     3,     4,   249,     6,    54,    54,    54,    54,
      52,   110,   111,    52,    41,    35,    36,    37,   117,   118,
     119,   305,   121,   122,   123,   124,   125,     4,     5,   274,
     129,   130,     3,     4,   133,   134,   135,   136,     3,     4,
     388,   140,   141,    22,    51,   144,   145,   146,   147,    51,
     148,    53,   151,   152,   153,     8,    51,   156,    53,   158,
     159,   160,   410,    53,     4,   164,   165,    51,    16,    53,
      18,    19,   170,   171,   319,   173,   554,    25,    26,    41,
     179,   180,   181,   328,    51,   184,    53,    16,   187,    18,
      19,   190,   191,    51,    20,    53,    25,    26,     4,    53,
     199,     0,     1,   202,   203,   204,    54,    52,     7,     8,
       9,    10,    11,     6,     6,   214,   215,    24,    17,     0,
       1,     6,   221,    22,     6,    54,   225,   226,     9,    10,
      11,     6,    12,    13,    14,    15,    17,     4,    16,     6,
      18,    19,    41,     6,     6,     0,     1,    25,    26,    29,
      31,    38,     6,    34,     9,    10,    11,     4,   257,     6,
      41,    41,    17,     6,     6,   263,   264,     4,   267,   268,
     269,   270,   271,   272,   273,    51,    31,    53,    52,    34,
      16,   426,    18,    19,   468,     4,    41,   286,    16,    25,
      18,    19,    16,     6,    18,    19,   294,    25,   296,    51,
     445,    25,    26,    42,    43,    44,    45,    46,    47,    48,
      49,    50,   311,   312,   313,   314,   315,   316,   317,     3,
       4,    51,     6,    53,   323,    51,    54,    53,     3,     4,
      54,     6,    54,   332,   333,   334,   335,   336,   337,   338,
     339,     4,    39,     6,   343,   344,   345,   346,   347,     6,
     348,   349,     3,     4,   353,     6,     3,     4,    54,     6,
       3,     4,    54,     6,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,    53,   376,   377,
     525,     4,    51,     6,    53,    51,    51,    53,    53,    53,
     389,   390,   391,   392,   393,   394,    52,    51,   397,   398,
     399,   400,    51,    51,    53,    53,    51,    51,    53,    53,
      51,    51,    53,    53,   413,   414,   415,   416,   417,   418,
      54,     4,   421,   422,   423,   424,    51,    51,    53,    53,
     429,   430,    51,     4,    53,     6,     4,    40,     6,   438,
     439,    53,     4,   442,     4,   443,   444,    52,    21,   447,
     448,    51,    54,    54,   453,   454,     4,     4,     4,    52,
      54,    52,    54,    52,    27,    53,    53,     4,    54,     4,
       6,    53,    28,     4,    53,    53,     4,    54,    54,     4,
      52,    22,   480,   481,    54,    52,     6,   485,   486,   634,
      54,   636,     6,    54,     6,    54,    54,    52,     6,    53,
     645,    54,    54,    54,   503,   504,   505,   506,   507,   508,
     509,   510,    54,    53,    53,    31,    54,     4,    52,    34,
      53,    53,   521,   522,    53,    52,    52,     4,     6,    52,
      54,    52,    54,     4,    53,    53,    53,     6,   537,   538,
     539,   540,   541,   542,   543,   544,     4,    54,    53,    32,
      54,    54,    54,    53,     4,     6,    52,    54,    53,    52,
     559,   560,   561,   562,   563,   564,   565,   566,    54,    54,
      54,     6,    52,    54,     6,    54,    52,     4,    53,    53,
     579,   580,   581,   582,   583,   584,   585,   586,    53,    53,
       4,    54,    54,    54,   593,   594,   595,   596,   597,   598,
       6,   599,   600,     6,     6,     6,   605,   606,   607,   608,
      54,   609,   610,    54,     4,    52,    54,     6,    52,     6,
     619,   620,   621,   622,    54,    54,     6,     6,    54,    54,
      53,     6,    54,    54,    54,    33,    52,    54,    53,     6,
       6,    54,     6,     6,     6,     6,     6,    52,     4,     6,
      54,     6,     4,    52,    54,   262,    52,    54,    52,    54,
     292,   235,    39,    28,   437,    57,    25,   501,   459,   613
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    56,    57,    58,    59,    60,    61,    63,    64,
       0,     7,    65,    17,   116,   117,   119,   121,   123,   125,
       1,     9,    10,    11,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    22,   144,
     145,   146,    51,     8,    66,    51,    53,    53,    51,    53,
      51,    53,    83,    41,   165,   116,    51,    53,   145,   148,
       4,    51,    53,    20,    67,    68,    69,    70,     4,     6,
     143,     4,     6,     4,     3,     4,   167,   168,   168,   168,
     168,    53,     4,     5,   147,   147,    23,    54,   149,   150,
      52,     6,     6,    51,    53,    24,    71,     6,     6,     6,
       6,    42,    43,    44,    45,    46,    47,    48,    49,    50,
     168,   168,   168,   168,     6,   168,   168,    51,    53,   168,
      54,   168,   168,   168,    51,    53,    38,    72,     6,     6,
     168,     6,   166,   168,   168,     4,     4,     6,    52,    54,
     168,   168,   168,   168,   168,   168,   168,   168,    51,    39,
      73,     6,     6,   168,   168,    54,   168,     6,   168,   168,
     168,    54,    53,    53,   168,   168,    52,    54,   168,   168,
     168,   168,   167,    51,    40,    74,    75,    76,    77,   168,
     168,   168,     4,    53,   168,     4,     6,   168,   168,   168,
     168,   168,     4,     4,    54,   167,   167,    52,   167,    51,
      21,    78,   168,   168,   168,    54,    95,   168,    54,     4,
       6,   168,     4,     4,   168,   168,    52,    54,    52,    54,
      52,   168,    53,    27,    79,   168,   168,   168,    53,    12,
      13,    14,    15,    29,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,     4,     6,
      54,     4,     6,   143,     4,   168,   168,   168,     4,     6,
      53,    28,    80,   168,   168,     4,   118,    51,    53,    51,
      53,    53,    51,    53,    53,    54,    98,   165,     4,   143,
      54,     4,    52,    54,    52,    54,   168,    54,    54,     6,
      51,    53,    62,    63,   167,     4,   167,    54,    16,    18,
      19,    25,    26,   127,   128,   129,   135,   136,   137,   141,
     142,   168,   168,   168,   168,   168,   168,   168,   143,     6,
      52,    54,    52,   168,    54,     6,     6,    81,   167,    54,
     167,    53,    51,    53,    53,    53,    51,    53,    51,    53,
      54,   128,   165,   168,   168,   168,   168,   168,   168,   168,
      54,   143,    52,   168,    52,    54,    31,   151,   152,   143,
      53,     4,   120,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   167,   167,    53,    52,
     168,    53,    34,   160,   161,    52,   122,    54,   127,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   167,     6,   167,   110,    52,    54,    53,    53,
     127,    53,    54,   168,   168,   168,   168,   168,   168,    52,
      54,   168,   168,   168,   168,     4,     6,     4,     6,    51,
      53,   114,   115,    53,    54,   126,    54,   124,   168,   168,
       6,   168,   168,   168,   168,   168,   168,   168,   168,    54,
     143,    54,     4,   168,   168,   111,   114,    32,   153,   154,
     155,    53,    16,    18,    19,    25,   130,   131,   132,   133,
     134,   138,   139,   140,   130,   168,   168,     4,     6,   168,
     167,   167,   143,     4,    54,   167,   167,    52,    54,   168,
     168,    30,    54,   112,    53,    54,   155,    35,    36,    37,
     162,   163,   164,    51,    53,    51,    53,    51,    53,    51,
      53,    54,   131,   165,    54,    52,    54,    54,     4,     6,
       6,   167,   167,    52,    54,   167,     4,   167,    52,    54,
      53,     6,    53,    53,    53,    54,   164,   168,   168,   168,
     168,   168,   168,   168,   168,    54,     4,     4,     6,   168,
     168,   143,    54,     4,   113,     6,     6,     6,     6,   168,
     168,   168,   168,   168,   168,   168,   168,    54,    54,     4,
      52,    54,    52,    54,   114,    54,     6,     6,     6,   168,
     168,   168,   168,   168,   168,   168,   168,    54,    54,    53,
      54,    54,     6,   168,   168,   168,   168,   168,   168,   168,
     168,   156,    54,   168,   168,   168,   168,   168,   168,   167,
     167,    33,   157,   158,   159,    52,    54,   168,   168,   168,
     168,   167,   167,    53,    54,   159,     6,     6,     6,   168,
     168,   168,   168,     6,     6,     6,     6,     6,     6,    52,
      54,    54,   143,     4,   143,     6,     6,    52,    54,    52,
     143,     4,    52,    54
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
      83,    86,    86,    86,    86,    86,    86,    87,    88,    89,
      90,    91,    92,    93,    93,    95,    94,    96,    96,    97,
      97,    98,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    99,    98,    98,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   110,   109,   111,   111,   113,   112,   114,
     114,   115,   115,   116,   116,   116,   116,   116,   118,   117,
     120,   119,   122,   121,   124,   123,   126,   125,   127,   127,
     128,   128,   128,   128,   128,   128,   128,   128,   128,   128,
     128,   129,   128,   130,   130,   131,   131,   131,   131,   131,
     131,   131,   131,   131,   132,   131,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   143,   144,   144,
     145,   146,   146,   147,   147,   148,   148,   148,   149,   150,
     151,   151,   152,   153,   153,   154,   154,   156,   155,   157,
     157,   158,   158,   159,   160,   160,   161,   162,   162,   163,
     163,   164,   164,   164,   165,   166,   166,   167,   167,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168
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
       1,     1,     1,     1,     1,     1,     1,    13,    11,    11,
      10,     9,     8,    10,    10,     0,    10,     1,     0,     1,
       2,     1,     1,     1,     1,     1,     1,     8,     1,     1,
       1,     0,     2,     1,    10,    10,     9,    12,    12,    11,
       8,     9,     9,     0,     9,     0,     2,     0,     5,     0,
       2,     4,     4,     1,     1,     1,     1,     1,     0,    12,
       0,    15,     0,    16,     0,    18,     0,    18,     1,     2,
       1,     1,     1,     1,     1,     8,     8,    10,    10,     5,
       5,     0,     2,     1,     2,     1,     1,     1,     1,     1,
       8,     8,    10,    10,     0,     2,    12,    12,    10,     9,
       8,    13,    13,    12,    11,    10,     1,     1,     1,     2,
       3,     6,     6,     1,     1,     0,     2,     2,     8,     8,
       1,     0,     6,     1,     0,     1,     2,     0,     9,     1,
       0,     1,     2,     4,     1,     0,     6,     1,     0,     1,
       2,     5,     5,     6,     5,     1,     0,     1,     1,     1,
       2,     2,     2,     2,     2,     2,     2,     2,     2
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
#line 1824 "parse_y.c" /* yacc.c:1652  */
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
#line 1851 "parse_y.c" /* yacc.c:1652  */
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
			compat_pstk_cop_grps = 0; /* reset preliminary number-of-copper-layers */
			}
#line 1894 "parse_y.c" /* yacc.c:1652  */
    break;

  case 8:
#line 236 "parse_y.y" /* yacc.c:1652  */
    { PreLoadElementPCB ();
		    layer_group_string = NULL; }
#line 1901 "parse_y.c" /* yacc.c:1652  */
    break;

  case 9:
#line 239 "parse_y.y" /* yacc.c:1652  */
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
#line 1925 "parse_y.c" /* yacc.c:1652  */
    break;

  case 10:
#line 261 "parse_y.y" /* yacc.c:1652  */
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
#line 1945 "parse_y.c" /* yacc.c:1652  */
    break;

  case 14:
#line 286 "parse_y.y" /* yacc.c:1652  */
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
#line 1963 "parse_y.c" /* yacc.c:1652  */
    break;

  case 15:
#line 300 "parse_y.y" /* yacc.c:1652  */
    {
				*yyFontkitValid = rnd_true;
				rnd_font_normalize_pcb_rnd(yyRndFont);
			}
#line 1972 "parse_y.c" /* yacc.c:1652  */
    break;

  case 17:
#line 309 "parse_y.y" /* yacc.c:1652  */
    {
  if (check_file_version ((yyvsp[-1].integer)) != 0)
    {
      YYABORT;
    }
}
#line 1983 "parse_y.c" /* yacc.c:1652  */
    break;

  case 18:
#line 319 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->hidlib.name = (yyvsp[-1].string);
				yyPCB->hidlib.dwg.X2 = RND_MAX_COORD;
				yyPCB->hidlib.dwg.Y2 = RND_MAX_COORD;
				old_fmt = 1;
			}
#line 1994 "parse_y.c" /* yacc.c:1652  */
    break;

  case 19:
#line 326 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->hidlib.name = (yyvsp[-3].string);
				yyPCB->hidlib.dwg.X2 = OU ((yyvsp[-2].measure));
				yyPCB->hidlib.dwg.Y2 = OU ((yyvsp[-1].measure));
				old_fmt = 1;
			}
#line 2005 "parse_y.c" /* yacc.c:1652  */
    break;

  case 20:
#line 333 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->hidlib.name = (yyvsp[-3].string);
				yyPCB->hidlib.dwg.X2 = NU ((yyvsp[-2].measure));
				yyPCB->hidlib.dwg.Y2 = NU ((yyvsp[-1].measure));
				old_fmt = 0;
			}
#line 2016 "parse_y.c" /* yacc.c:1652  */
    break;

  case 24:
#line 348 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->hidlib.grid = OU ((yyvsp[-3].measure));
				yyPCB->hidlib.grid_ox = OU ((yyvsp[-2].measure));
				yyPCB->hidlib.grid_oy = OU ((yyvsp[-1].measure));
			}
#line 2026 "parse_y.c" /* yacc.c:1652  */
    break;

  case 25:
#line 356 "parse_y.y" /* yacc.c:1652  */
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
#line 2042 "parse_y.c" /* yacc.c:1652  */
    break;

  case 26:
#line 371 "parse_y.y" /* yacc.c:1652  */
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
#line 2058 "parse_y.c" /* yacc.c:1652  */
    break;

  case 27:
#line 386 "parse_y.y" /* yacc.c:1652  */
    {
/* Not loading cursor position and zoom anymore */
			}
#line 2066 "parse_y.c" /* yacc.c:1652  */
    break;

  case 28:
#line 390 "parse_y.y" /* yacc.c:1652  */
    {
/* Not loading cursor position and zoom anymore */
			}
#line 2074 "parse_y.c" /* yacc.c:1652  */
    break;

  case 31:
#line 399 "parse_y.y" /* yacc.c:1652  */
    {
				/* Read in cmil^2 for now; in future this should be a noop. */
				load_meta_float("design/poly_isle_area", RND_MIL_TO_COORD(RND_MIL_TO_COORD ((yyvsp[-1].number)) / 100.0) / 100.0);
			}
#line 2083 "parse_y.c" /* yacc.c:1652  */
    break;

  case 33:
#line 408 "parse_y.y" /* yacc.c:1652  */
    {
				yyPCB->ThermScale = (yyvsp[-1].number);
				if (yyPCB->ThermScale < 0.01)
					rnd_message(RND_MSG_ERROR, "Your ThermalScale is too small. This will probably cause problems in calculating thermals.\n");
			}
#line 2093 "parse_y.c" /* yacc.c:1652  */
    break;

  case 38:
#line 424 "parse_y.y" /* yacc.c:1652  */
    {
				load_meta_coord("design/bloat", NU((yyvsp[-3].measure)));
				load_meta_coord("design/shrink", NU((yyvsp[-2].measure)));
				load_meta_coord("design/min_wid", NU((yyvsp[-1].measure)));
				load_meta_coord("design/min_ring", NU((yyvsp[-1].measure)));
			}
#line 2104 "parse_y.c" /* yacc.c:1652  */
    break;

  case 39:
#line 434 "parse_y.y" /* yacc.c:1652  */
    {
				load_meta_coord("design/bloat", NU((yyvsp[-4].measure)));
				load_meta_coord("design/shrink", NU((yyvsp[-3].measure)));
				load_meta_coord("design/min_wid", NU((yyvsp[-2].measure)));
				load_meta_coord("design/min_slk", NU((yyvsp[-1].measure)));
				load_meta_coord("design/min_ring", NU((yyvsp[-2].measure)));
			}
#line 2116 "parse_y.c" /* yacc.c:1652  */
    break;

  case 40:
#line 445 "parse_y.y" /* yacc.c:1652  */
    {
				load_meta_coord("design/bloat", NU((yyvsp[-6].measure)));
				load_meta_coord("design/shrink", NU((yyvsp[-5].measure)));
				load_meta_coord("design/min_wid", NU((yyvsp[-4].measure)));
				load_meta_coord("design/min_slk", NU((yyvsp[-3].measure)));
				load_meta_coord("design/min_drill", NU((yyvsp[-2].measure)));
				load_meta_coord("design/min_ring", NU((yyvsp[-1].measure)));
			}
#line 2129 "parse_y.c" /* yacc.c:1652  */
    break;

  case 41:
#line 457 "parse_y.y" /* yacc.c:1652  */
    {
				yy_pcb_flags = pcb_flag_make((yyvsp[-1].integer) & PCB_FLAGS);
			}
#line 2137 "parse_y.c" /* yacc.c:1652  */
    break;

  case 42:
#line 461 "parse_y.y" /* yacc.c:1652  */
    {
			  yy_pcb_flags = pcb_strflg_board_s2f((yyvsp[-1].string), yyerror);
			  free((yyvsp[-1].string));
			}
#line 2146 "parse_y.c" /* yacc.c:1652  */
    break;

  case 44:
#line 470 "parse_y.y" /* yacc.c:1652  */
    {
				const char *s;

				layer_group_string = (yyvsp[-1].string);

				/* count number of copper layers for bbvia offseting (negative bbvia values internally) */
				compat_pstk_cop_grps = 1;
				for(s = (yyvsp[-1].string); *s != '\0'; s++)
					if (*s == ':')
						compat_pstk_cop_grps++;
			}
#line 2162 "parse_y.c" /* yacc.c:1652  */
    break;

  case 46:
#line 486 "parse_y.y" /* yacc.c:1652  */
    {
				if (pcb_route_string_parse(yyPCB->Data, (yyvsp[-1].string), &yyPCB->RouteStyle, "mil"))
				{
					rnd_message(RND_MSG_ERROR, "illegal route-style string\n");
					YYABORT;
				}
				free((yyvsp[-1].string));
			}
#line 2175 "parse_y.c" /* yacc.c:1652  */
    break;

  case 47:
#line 495 "parse_y.y" /* yacc.c:1652  */
    {
				if (pcb_route_string_parse(yyPCB->Data, ((yyvsp[-1].string) == NULL ? "" : (yyvsp[-1].string)), &yyPCB->RouteStyle, "cmil"))
				{
					rnd_message(RND_MSG_ERROR, "illegal route-style string\n");
					YYABORT;
				}
				free((yyvsp[-1].string));
			}
#line 2188 "parse_y.c" /* yacc.c:1652  */
    break;

  case 54:
#line 518 "parse_y.y" /* yacc.c:1652  */
    { attr_list = & yyPCB->Attributes; }
#line 2194 "parse_y.c" /* yacc.c:1652  */
    break;

  case 58:
#line 522 "parse_y.y" /* yacc.c:1652  */
    {
					/* clear pointer to force memory allocation by
					 * the appropriate subroutine
					 */
				yysubc = NULL;
			}
#line 2205 "parse_y.c" /* yacc.c:1652  */
    break;

  case 60:
#line 529 "parse_y.y" /* yacc.c:1652  */
    { YYABORT; }
#line 2211 "parse_y.c" /* yacc.c:1652  */
    break;

  case 67:
#line 544 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_old_via_new_bb (yyData, -1, NU ((yyvsp[-10].measure)), NU ((yyvsp[-9].measure)), NU ((yyvsp[-8].measure)), NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)),
						     NU ((yyvsp[-5].measure)), (yyvsp[-2].string), (yyvsp[-1].flagtype), (yyvsp[-4].integer), (yyvsp[-3].integer));
				free ((yyvsp[-2].string));
			}
#line 2221 "parse_y.c" /* yacc.c:1652  */
    break;

  case 68:
#line 554 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_old_via_new(yyData, -1, NU ((yyvsp[-8].measure)), NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)),
				                     NU ((yyvsp[-3].measure)), (yyvsp[-2].string), (yyvsp[-1].flagtype));
				free ((yyvsp[-2].string));
			}
#line 2231 "parse_y.c" /* yacc.c:1652  */
    break;

  case 69:
#line 564 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_old_via_new(yyData, -1, OU ((yyvsp[-8].measure)), OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-2].string),
					pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2241 "parse_y.c" /* yacc.c:1652  */
    break;

  case 70:
#line 575 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_old_via_new(yyData, -1, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)),
					     OU ((yyvsp[-5].measure)) + OU((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2251 "parse_y.c" /* yacc.c:1652  */
    break;

  case 71:
#line 585 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_old_via_new(yyData, -1, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU((yyvsp[-4].measure)) + 2*PCB_MASKFRAME,  OU ((yyvsp[-3].measure)), (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2261 "parse_y.c" /* yacc.c:1652  */
    break;

  case 72:
#line 595 "parse_y.y" /* yacc.c:1652  */
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
#line 2278 "parse_y.c" /* yacc.c:1652  */
    break;

  case 73:
#line 611 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_rat_new(yyData, -1, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-4].measure)), NU ((yyvsp[-3].measure)), (yyvsp[-5].integer), (yyvsp[-2].integer),
					conf_core.appearance.rat_thickness, (yyvsp[-1].flagtype), NULL, NULL);
			}
#line 2287 "parse_y.c" /* yacc.c:1652  */
    break;

  case 74:
#line 616 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_rat_new(yyData, -1, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-5].integer), (yyvsp[-2].integer),
					conf_core.appearance.rat_thickness, pcb_flag_old((yyvsp[-1].integer)), NULL, NULL);
			}
#line 2296 "parse_y.c" /* yacc.c:1652  */
    break;

  case 75:
#line 625 "parse_y.y" /* yacc.c:1652  */
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
#line 2332 "parse_y.c" /* yacc.c:1652  */
    break;

  case 87:
#line 678 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_poly_new_from_rectangle(Layer,
					OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-5].measure)) + OU ((yyvsp[-3].measure)), OU ((yyvsp[-4].measure)) + OU ((yyvsp[-2].measure)), 0, pcb_flag_old((yyvsp[-1].integer)));
			}
#line 2341 "parse_y.c" /* yacc.c:1652  */
    break;

  case 91:
#line 685 "parse_y.y" /* yacc.c:1652  */
    { attr_list = & Layer->Attributes; }
#line 2347 "parse_y.c" /* yacc.c:1652  */
    break;

  case 94:
#line 691 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_line_new(Layer, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)),
				                            NU ((yyvsp[-3].measure)), NU ((yyvsp[-2].measure)), (yyvsp[-1].flagtype));
			}
#line 2356 "parse_y.c" /* yacc.c:1652  */
    break;

  case 95:
#line 700 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_line_new(Layer, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)),
						     OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)), pcb_flag_old((yyvsp[-1].integer)));
			}
#line 2365 "parse_y.c" /* yacc.c:1652  */
    break;

  case 96:
#line 709 "parse_y.y" /* yacc.c:1652  */
    {
				/* eliminate old-style rat-lines */
			if ((IV ((yyvsp[-1].measure)) & PCB_FLAG_RAT) == 0)
				pcb_line_new(Layer, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)),
					200*PCB_GROUNDPLANEFRAME, pcb_flag_old(IV ((yyvsp[-1].measure))));
			}
#line 2376 "parse_y.c" /* yacc.c:1652  */
    break;

  case 97:
#line 720 "parse_y.y" /* yacc.c:1652  */
    {
			  pcb_arc_new(Layer, NU ((yyvsp[-9].measure)), NU ((yyvsp[-8].measure)), NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), (yyvsp[-3].number), (yyvsp[-2].number),
			                             NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-1].flagtype), rnd_true);
			}
#line 2385 "parse_y.c" /* yacc.c:1652  */
    break;

  case 98:
#line 729 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_arc_new(Layer, OU ((yyvsp[-9].measure)), OU ((yyvsp[-8].measure)), OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), (yyvsp[-3].number), (yyvsp[-2].number),
						    OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), pcb_flag_old((yyvsp[-1].integer)), rnd_true);
			}
#line 2394 "parse_y.c" /* yacc.c:1652  */
    break;

  case 99:
#line 738 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_arc_new(Layer, OU ((yyvsp[-8].measure)), OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-6].measure)), IV ((yyvsp[-3].measure)), (yyvsp[-2].number),
					OU ((yyvsp[-4].measure)), 200*PCB_GROUNDPLANEFRAME, pcb_flag_old((yyvsp[-1].integer)), rnd_true);
			}
#line 2403 "parse_y.c" /* yacc.c:1652  */
    break;

  case 100:
#line 747 "parse_y.y" /* yacc.c:1652  */
    {
					/* use a default scale of 100% */
				pcb_text_new(Layer,yyRndFont,OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].number) * 90.0, 100, 0, (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2413 "parse_y.c" /* yacc.c:1652  */
    break;

  case 101:
#line 757 "parse_y.y" /* yacc.c:1652  */
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
#line 2432 "parse_y.c" /* yacc.c:1652  */
    break;

  case 102:
#line 775 "parse_y.y" /* yacc.c:1652  */
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
#line 2461 "parse_y.c" /* yacc.c:1652  */
    break;

  case 103:
#line 804 "parse_y.y" /* yacc.c:1652  */
    {
				Polygon = pcb_poly_new(Layer, 0, (yyvsp[-2].flagtype));
			}
#line 2469 "parse_y.c" /* yacc.c:1652  */
    break;

  case 104:
#line 809 "parse_y.y" /* yacc.c:1652  */
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
#line 2505 "parse_y.c" /* yacc.c:1652  */
    break;

  case 107:
#line 849 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_poly_hole_new(Polygon);
			}
#line 2513 "parse_y.c" /* yacc.c:1652  */
    break;

  case 111:
#line 863 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_poly_point_new(Polygon, OU ((yyvsp[-2].measure)), OU ((yyvsp[-1].measure)));
			}
#line 2521 "parse_y.c" /* yacc.c:1652  */
    break;

  case 112:
#line 867 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_poly_point_new(Polygon, NU ((yyvsp[-2].measure)), NU ((yyvsp[-1].measure)));
			}
#line 2529 "parse_y.c" /* yacc.c:1652  */
    break;

  case 118:
#line 885 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc = io_pcb_element_new(yyData, yysubc, yyRndFont, pcb_no_flags(),
					(yyvsp[-6].string), (yyvsp[-5].string), NULL, OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), (yyvsp[-2].integer), 100, pcb_no_flags(), rnd_false);
				free ((yyvsp[-6].string));
				free ((yyvsp[-5].string));
				pin_num = 1;
			}
#line 2541 "parse_y.c" /* yacc.c:1652  */
    break;

  case 119:
#line 893 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_fin(yyData);
			}
#line 2549 "parse_y.c" /* yacc.c:1652  */
    break;

  case 120:
#line 903 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc = io_pcb_element_new(yyData, yysubc, yyRndFont, pcb_flag_old((yyvsp[-9].integer)),
					(yyvsp[-8].string), (yyvsp[-7].string), NULL, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), IV ((yyvsp[-4].measure)), IV ((yyvsp[-3].measure)), pcb_flag_old((yyvsp[-2].integer)), rnd_false);
				free ((yyvsp[-8].string));
				free ((yyvsp[-7].string));
				pin_num = 1;
			}
#line 2561 "parse_y.c" /* yacc.c:1652  */
    break;

  case 121:
#line 911 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_fin(yyData);
			}
#line 2569 "parse_y.c" /* yacc.c:1652  */
    break;

  case 122:
#line 921 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc = io_pcb_element_new(yyData, yysubc, yyRndFont, pcb_flag_old((yyvsp[-10].integer)),
					(yyvsp[-9].string), (yyvsp[-8].string), (yyvsp[-7].string), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), IV ((yyvsp[-4].measure)), IV ((yyvsp[-3].measure)), pcb_flag_old((yyvsp[-2].integer)), rnd_false);
				free ((yyvsp[-9].string));
				free ((yyvsp[-8].string));
				free ((yyvsp[-7].string));
				pin_num = 1;
			}
#line 2582 "parse_y.c" /* yacc.c:1652  */
    break;

  case 123:
#line 930 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_fin(yyData);
			}
#line 2590 "parse_y.c" /* yacc.c:1652  */
    break;

  case 124:
#line 941 "parse_y.y" /* yacc.c:1652  */
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
#line 2605 "parse_y.c" /* yacc.c:1652  */
    break;

  case 125:
#line 952 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_fin(yyData);
			}
#line 2613 "parse_y.c" /* yacc.c:1652  */
    break;

  case 126:
#line 963 "parse_y.y" /* yacc.c:1652  */
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
#line 2628 "parse_y.c" /* yacc.c:1652  */
    break;

  case 127:
#line 974 "parse_y.y" /* yacc.c:1652  */
    {
				if (pcb_subc_is_empty(yysubc)) {
					pcb_subc_free(yysubc);
					yysubc = NULL;
				}
				else {
					io_pcb_element_fin(yyData);
				}
			}
#line 2642 "parse_y.c" /* yacc.c:1652  */
    break;

  case 135:
#line 998 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_line_new(yysubc, NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), NU ((yyvsp[-3].measure)), NU ((yyvsp[-2].measure)), NU ((yyvsp[-1].measure)));
			}
#line 2650 "parse_y.c" /* yacc.c:1652  */
    break;

  case 136:
#line 1003 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_line_new(yysubc, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)), OU ((yyvsp[-1].measure)));
			}
#line 2658 "parse_y.c" /* yacc.c:1652  */
    break;

  case 137:
#line 1008 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_arc_new(yysubc, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), NU ((yyvsp[-1].measure)));
			}
#line 2666 "parse_y.c" /* yacc.c:1652  */
    break;

  case 138:
#line 1013 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_arc_new(yysubc, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), OU ((yyvsp[-1].measure)));
			}
#line 2674 "parse_y.c" /* yacc.c:1652  */
    break;

  case 139:
#line 1018 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc_ox = NU ((yyvsp[-2].measure));
				yysubc_oy = NU ((yyvsp[-1].measure));
			}
#line 2683 "parse_y.c" /* yacc.c:1652  */
    break;

  case 140:
#line 1023 "parse_y.y" /* yacc.c:1652  */
    {
				yysubc_ox = OU ((yyvsp[-2].measure));
				yysubc_oy = OU ((yyvsp[-1].measure));
			}
#line 2692 "parse_y.c" /* yacc.c:1652  */
    break;

  case 141:
#line 1027 "parse_y.y" /* yacc.c:1652  */
    { attr_list = & yysubc->Attributes; }
#line 2698 "parse_y.c" /* yacc.c:1652  */
    break;

  case 150:
#line 1043 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_line_new(yysubc, NU ((yyvsp[-5].measure)) + yysubc_ox,
					NU ((yyvsp[-4].measure)) + yysubc_oy, NU ((yyvsp[-3].measure)) + yysubc_ox,
					NU ((yyvsp[-2].measure)) + yysubc_oy, NU ((yyvsp[-1].measure)));
			}
#line 2708 "parse_y.c" /* yacc.c:1652  */
    break;

  case 151:
#line 1049 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_line_new(yysubc, OU ((yyvsp[-5].measure)) + yysubc_ox,
					OU ((yyvsp[-4].measure)) + yysubc_oy, OU ((yyvsp[-3].measure)) + yysubc_ox,
					OU ((yyvsp[-2].measure)) + yysubc_oy, OU ((yyvsp[-1].measure)));
			}
#line 2718 "parse_y.c" /* yacc.c:1652  */
    break;

  case 152:
#line 1056 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_arc_new(yysubc, NU ((yyvsp[-7].measure)) + yysubc_ox,
					NU ((yyvsp[-6].measure)) + yysubc_oy, NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), NU ((yyvsp[-1].measure)));
			}
#line 2727 "parse_y.c" /* yacc.c:1652  */
    break;

  case 153:
#line 1061 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_arc_new(yysubc, OU ((yyvsp[-7].measure)) + yysubc_ox,
					OU ((yyvsp[-6].measure)) + yysubc_oy, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].number), (yyvsp[-2].number), OU ((yyvsp[-1].measure)));
			}
#line 2736 "parse_y.c" /* yacc.c:1652  */
    break;

  case 154:
#line 1065 "parse_y.y" /* yacc.c:1652  */
    { attr_list = & yysubc->Attributes; }
#line 2742 "parse_y.c" /* yacc.c:1652  */
    break;

  case 156:
#line 1072 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_pstk_t *pin = io_pcb_element_pin_new(yysubc, NU ((yyvsp[-9].measure)) + yysubc_ox,
					NU ((yyvsp[-8].measure)) + yysubc_oy, NU ((yyvsp[-7].measure)), NU ((yyvsp[-6].measure)), NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), (yyvsp[-3].string),
					(yyvsp[-2].string), (yyvsp[-1].flagtype));
				pcb_attrib_compat_set_intconn(&pin->Attributes, yy_intconn);
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2755 "parse_y.c" /* yacc.c:1652  */
    break;

  case 157:
#line 1085 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_pin_new(yysubc, OU ((yyvsp[-9].measure)) + yysubc_ox,
					OU ((yyvsp[-8].measure)) + yysubc_oy, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), (yyvsp[-3].string),
					(yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2767 "parse_y.c" /* yacc.c:1652  */
    break;

  case 158:
#line 1097 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_pin_new(yysubc, OU ((yyvsp[-7].measure)), OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU ((yyvsp[-5].measure)) + 2*PCB_MASKFRAME, OU ((yyvsp[-4].measure)), (yyvsp[-3].string), (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2778 "parse_y.c" /* yacc.c:1652  */
    break;

  case 159:
#line 1108 "parse_y.y" /* yacc.c:1652  */
    {
				char	p_number[8];

				sprintf(p_number, "%d", pin_num++);
				io_pcb_element_pin_new(yysubc, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU ((yyvsp[-4].measure)) + 2*PCB_MASKFRAME, OU ((yyvsp[-3].measure)), (yyvsp[-2].string), p_number, pcb_flag_old((yyvsp[-1].integer)));

				free ((yyvsp[-2].string));
			}
#line 2792 "parse_y.c" /* yacc.c:1652  */
    break;

  case 160:
#line 1124 "parse_y.y" /* yacc.c:1652  */
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
#line 2811 "parse_y.c" /* yacc.c:1652  */
    break;

  case 161:
#line 1143 "parse_y.y" /* yacc.c:1652  */
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
#line 2826 "parse_y.c" /* yacc.c:1652  */
    break;

  case 162:
#line 1158 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_pad_new(yysubc,OU ((yyvsp[-10].measure)) + yysubc_ox,
					OU ((yyvsp[-9].measure)) + yysubc_oy, OU ((yyvsp[-8].measure)) + yysubc_ox,
					OU ((yyvsp[-7].measure)) + yysubc_oy, OU ((yyvsp[-6].measure)), OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)),
					(yyvsp[-3].string), (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2839 "parse_y.c" /* yacc.c:1652  */
    break;

  case 163:
#line 1171 "parse_y.y" /* yacc.c:1652  */
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
#line 2863 "parse_y.c" /* yacc.c:1652  */
    break;

  case 164:
#line 1195 "parse_y.y" /* yacc.c:1652  */
    {
				io_pcb_element_pad_new(yysubc,OU ((yyvsp[-8].measure)),OU ((yyvsp[-7].measure)),OU ((yyvsp[-6].measure)),OU ((yyvsp[-5].measure)),OU ((yyvsp[-4].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU ((yyvsp[-4].measure)) + 2*PCB_MASKFRAME, (yyvsp[-3].string), (yyvsp[-2].string), pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2874 "parse_y.c" /* yacc.c:1652  */
    break;

  case 165:
#line 1206 "parse_y.y" /* yacc.c:1652  */
    {
				char		p_number[8];

				sprintf(p_number, "%d", pin_num++);
				io_pcb_element_pad_new(yysubc,OU ((yyvsp[-7].measure)),OU ((yyvsp[-6].measure)),OU ((yyvsp[-5].measure)),OU ((yyvsp[-4].measure)),OU ((yyvsp[-3].measure)), 2*PCB_GROUNDPLANEFRAME,
					OU ((yyvsp[-3].measure)) + 2*PCB_MASKFRAME, (yyvsp[-2].string),p_number, pcb_flag_old((yyvsp[-1].integer)));
				free ((yyvsp[-2].string));
			}
#line 2887 "parse_y.c" /* yacc.c:1652  */
    break;

  case 166:
#line 1216 "parse_y.y" /* yacc.c:1652  */
    { (yyval.flagtype) = pcb_flag_old((yyvsp[0].integer)); }
#line 2893 "parse_y.c" /* yacc.c:1652  */
    break;

  case 167:
#line 1217 "parse_y.y" /* yacc.c:1652  */
    { (yyval.flagtype) = pcb_strflg_s2f((yyvsp[0].string), yyerror, &yy_intconn, 1); free((yyvsp[0].string)); }
#line 2899 "parse_y.c" /* yacc.c:1652  */
    break;

  case 171:
#line 1228 "parse_y.y" /* yacc.c:1652  */
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
#line 2919 "parse_y.c" /* yacc.c:1652  */
    break;

  case 172:
#line 1244 "parse_y.y" /* yacc.c:1652  */
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
#line 2939 "parse_y.c" /* yacc.c:1652  */
    break;

  case 178:
#line 1275 "parse_y.y" /* yacc.c:1652  */
    {
				rnd_font_new_line_in_glyph(Glyph, OU ((yyvsp[-5].measure)), OU ((yyvsp[-4].measure)), OU ((yyvsp[-3].measure)), OU ((yyvsp[-2].measure)), OU ((yyvsp[-1].measure)));
			}
#line 2947 "parse_y.c" /* yacc.c:1652  */
    break;

  case 179:
#line 1282 "parse_y.y" /* yacc.c:1652  */
    {
				rnd_font_new_line_in_glyph(Glyph, NU ((yyvsp[-5].measure)), NU ((yyvsp[-4].measure)), NU ((yyvsp[-3].measure)), NU ((yyvsp[-2].measure)), NU ((yyvsp[-1].measure)));
			}
#line 2955 "parse_y.c" /* yacc.c:1652  */
    break;

  case 187:
#line 1309 "parse_y.y" /* yacc.c:1652  */
    {
				currnet = pcb_net_get(yyPCB, &yyPCB->netlist[PCB_NETLIST_INPUT], (yyvsp[-3].string), PCB_NETA_ALLOC);
				if (((yyvsp[-2].string) != NULL) && (*(yyvsp[-2].string) != '\0'))
					pcb_attribute_put(&currnet->Attributes, "style", (yyvsp[-2].string));
				free ((yyvsp[-3].string));
				free ((yyvsp[-2].string));
			}
#line 2967 "parse_y.c" /* yacc.c:1652  */
    break;

  case 193:
#line 1331 "parse_y.y" /* yacc.c:1652  */
    {
				pcb_net_term_get_by_pinname(currnet, (yyvsp[-1].string), 1);
				free ((yyvsp[-1].string));
			}
#line 2976 "parse_y.c" /* yacc.c:1652  */
    break;

  case 201:
#line 1378 "parse_y.y" /* yacc.c:1652  */
    { pcb_ratspatch_append(yyPCB, RATP_ADD_CONN, (yyvsp[-2].string), (yyvsp[-1].string), NULL, 0); free((yyvsp[-2].string)); free((yyvsp[-1].string)); }
#line 2982 "parse_y.c" /* yacc.c:1652  */
    break;

  case 202:
#line 1379 "parse_y.y" /* yacc.c:1652  */
    { pcb_ratspatch_append(yyPCB, RATP_DEL_CONN, (yyvsp[-2].string), (yyvsp[-1].string), NULL, 0); free((yyvsp[-2].string)); free((yyvsp[-1].string)); }
#line 2988 "parse_y.c" /* yacc.c:1652  */
    break;

  case 203:
#line 1380 "parse_y.y" /* yacc.c:1652  */
    { pcb_ratspatch_append(yyPCB, RATP_CHANGE_COMP_ATTRIB, (yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].string), 0); free((yyvsp[-3].string)); free((yyvsp[-2].string)); free((yyvsp[-1].string)); }
#line 2994 "parse_y.c" /* yacc.c:1652  */
    break;

  case 204:
#line 1385 "parse_y.y" /* yacc.c:1652  */
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
#line 3009 "parse_y.c" /* yacc.c:1652  */
    break;

  case 205:
#line 1397 "parse_y.y" /* yacc.c:1652  */
    { (yyval.string) = (yyvsp[0].string); }
#line 3015 "parse_y.c" /* yacc.c:1652  */
    break;

  case 206:
#line 1398 "parse_y.y" /* yacc.c:1652  */
    { (yyval.string) = 0; }
#line 3021 "parse_y.c" /* yacc.c:1652  */
    break;

  case 207:
#line 1402 "parse_y.y" /* yacc.c:1652  */
    { (yyval.number) = (yyvsp[0].number); }
#line 3027 "parse_y.c" /* yacc.c:1652  */
    break;

  case 208:
#line 1403 "parse_y.y" /* yacc.c:1652  */
    { (yyval.number) = (yyvsp[0].integer); }
#line 3033 "parse_y.c" /* yacc.c:1652  */
    break;

  case 209:
#line 1408 "parse_y.y" /* yacc.c:1652  */
    { do_measure(&(yyval.measure), (yyvsp[0].number), RND_MIL_TO_COORD ((yyvsp[0].number)) / 100.0, 0); }
#line 3039 "parse_y.c" /* yacc.c:1652  */
    break;

  case 210:
#line 1409 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MIL_TO_COORD ((yyvsp[-1].number)) / 100000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3045 "parse_y.c" /* yacc.c:1652  */
    break;

  case 211:
#line 1410 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MIL_TO_COORD ((yyvsp[-1].number)) / 100.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3051 "parse_y.c" /* yacc.c:1652  */
    break;

  case 212:
#line 1411 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MIL_TO_COORD ((yyvsp[-1].number))); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3057 "parse_y.c" /* yacc.c:1652  */
    break;

  case 213:
#line 1412 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_INCH_TO_COORD ((yyvsp[-1].number))); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3063 "parse_y.c" /* yacc.c:1652  */
    break;

  case 214:
#line 1413 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MM_TO_COORD ((yyvsp[-1].number)) / 1000000.0); pcb_io_pcb_usty_seen |= PCB_USTY_NANOMETER; }
#line 3069 "parse_y.c" /* yacc.c:1652  */
    break;

  case 215:
#line 1414 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MM_TO_COORD ((yyvsp[-1].number)) / 1000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3075 "parse_y.c" /* yacc.c:1652  */
    break;

  case 216:
#line 1415 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MM_TO_COORD ((yyvsp[-1].number))); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3081 "parse_y.c" /* yacc.c:1652  */
    break;

  case 217:
#line 1416 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MM_TO_COORD ((yyvsp[-1].number)) * 1000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3087 "parse_y.c" /* yacc.c:1652  */
    break;

  case 218:
#line 1417 "parse_y.y" /* yacc.c:1652  */
    { M ((yyval.measure), (yyvsp[-1].number), RND_MM_TO_COORD ((yyvsp[-1].number)) * 1000000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
#line 3093 "parse_y.c" /* yacc.c:1652  */
    break;


#line 3097 "parse_y.c" /* yacc.c:1652  */
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
#line 1420 "parse_y.y" /* yacc.c:1918  */


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
