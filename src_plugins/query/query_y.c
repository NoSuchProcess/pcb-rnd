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


/* Substitute the variable and function names.  */
#define yyparse         qry_parse
#define yylex           qry_lex
#define yyerror         qry_error
#define yydebug         qry_debug
#define yynerrs         qry_nerrs

#define yylval          qry_lval
#define yychar          qry_char

/* Copy the first part of user declarations.  */
#line 1 "query_y.y" /* yacc.c:339  */

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Query language - compiler: grammar */

#include <assert.h>
#include "unit.h"
#include "query.h"
#include "query_l.h"
#include "compat_misc.h"
#include "flag_str.h"
#include "fields_sphash.h"

#define UNIT_CONV(dst, negative, val, unit) \
do { \
	dst = val; \
	if (negative) \
		dst = -dst; \
	if (unit != NULL) { \
		if (unit->family == PCB_UNIT_IMPERIAL) \
			dst = PCB_MIL_TO_COORD(dst); \
		else if (unit->family == PCB_UNIT_METRIC) \
			dst = PCB_MM_TO_COORD(dst); \
		dst /= unit->scale_factor; \
	} \
} while(0)

#define BINOP(dst, op1, operator, op2) \
do { \
	assert(op2->next == NULL); \
	assert(op2->next == NULL); \
	dst = pcb_qry_n_alloc(operator); \
	pcb_qry_n_insert(dst, op2); \
	pcb_qry_n_insert(dst, op1); \
} while(0)

#define UNOP(dst, operator, op) \
do { \
	assert(op->next == NULL); \
	dst = pcb_qry_n_alloc(operator); \
	pcb_qry_n_insert(dst, op); \
} while(0)

static pcb_query_iter_t *iter_ctx;

static char *attrib_prepend_free(char *orig, char *prep, char sep)
{
	int l1 = strlen(orig), l2 = strlen(prep);
	char *res = malloc(l1+l2+2);
	memcpy(res, prep, l2);
	res[l2] = sep;
	memcpy(res+l2+1, orig, l1+1);
	free(orig);
	free(prep);
	return res;
}

static pcb_qry_node_t *make_regex_free(char *str)
{
	pcb_qry_node_t *res = pcb_qry_n_alloc(PCBQ_DATA_REGEX);
	res->data.str = str;
	res->precomp.regex = re_se_comp(str);
	if (res->precomp.regex == NULL)
		yyerror(NULL, "Invalid regex\n");
	return res;
}


static pcb_qry_node_t *make_flag_free(char *str)
{
	const pcb_flag_bits_t *i = pcb_strflg_name(str, 0x7FFFFFFF);
	pcb_qry_node_t *nd;

	if (i == NULL) {
		yyerror(NULL, "Unknown flag");
		free(str);
		return NULL;
	}

	nd = pcb_qry_n_alloc(PCBQ_FLAG);
	nd->precomp.flg = i;
	free(str);
	return nd;
}



#line 187 "query_y.c" /* yacc.c:339  */

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
   by #include "query_y.h".  */
#ifndef YY_QRY_QUERY_Y_H_INCLUDED
# define YY_QRY_QUERY_Y_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int qry_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    T_LET = 258,
    T_ASSERT = 259,
    T_RULE = 260,
    T_LIST = 261,
    T_INVALID = 262,
    T_FLD_P = 263,
    T_FLD_A = 264,
    T_FLD_FLAG = 265,
    T_OR = 266,
    T_AND = 267,
    T_EQ = 268,
    T_NEQ = 269,
    T_GTEQ = 270,
    T_LTEQ = 271,
    T_NL = 272,
    T_UNIT = 273,
    T_STR = 274,
    T_QSTR = 275,
    T_INT = 276,
    T_DBL = 277,
    T_CONST = 278
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 118 "query_y.y" /* yacc.c:355  */

	char *s;
	pcb_coord_t c;
	double d;
	const pcb_unit_t *u;
	pcb_qry_node_t *n;

#line 259 "query_y.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE qry_lval;

int qry_parse (pcb_qry_node_t **prg_out);

#endif /* !YY_QRY_QUERY_Y_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 274 "query_y.c" /* yacc.c:358  */

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
#define YYFINAL  9
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   228

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  37
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  18
/* YYNRULES -- Number of rules.  */
#define YYNRULES  58
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  95

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   278

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    31,     2,     2,     2,     2,     2,     2,
      32,    33,    28,    26,    36,    27,    30,    29,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      24,     2,    25,     2,    35,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,    34,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    23
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   156,   156,   157,   162,   162,   175,   176,   180,   192,
     193,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   231,   232,   233,   234,   238,   242,
     243,   247,   248,   249,   250,   251,   255,   256,   257,   261,
     262,   263,   267,   268,   272,   286,   287,   291,   292
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_LET", "T_ASSERT", "T_RULE", "T_LIST",
  "T_INVALID", "T_FLD_P", "T_FLD_A", "T_FLD_FLAG", "T_OR", "T_AND", "T_EQ",
  "T_NEQ", "T_GTEQ", "T_LTEQ", "T_NL", "T_UNIT", "T_STR", "T_QSTR",
  "T_INT", "T_DBL", "T_CONST", "'<'", "'>'", "'+'", "'-'", "'*'", "'/'",
  "'.'", "'!'", "'('", "')'", "'~'", "'@'", "','", "$accept", "program",
  "program_expr", "$@1", "program_rules", "rule", "exprs", "expr",
  "number", "string_literal", "maybe_unit", "fields", "attribs", "var",
  "fcall", "fname", "fargs", "words", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,    60,    62,    43,    45,    42,    47,
      46,    33,    40,    41,   126,    64,    44
};
# endif

#define YYPACT_NINF -64

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-64)))

#define YYTABLE_NINF -55

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      26,   -18,    12,   -64,    40,   -64,    20,   -18,    10,   -64,
       1,   -64,    16,   -64,    34,    34,   -64,   -11,    40,    40,
     -64,   139,   -64,   -64,    43,   -64,    42,   -64,   -64,   -64,
      47,   -64,   -64,   -64,    34,    34,    51,    91,    40,    40,
      40,    40,    40,    40,    40,    40,    40,    40,    40,    40,
      46,    49,    -3,    40,    53,   -64,   -64,   -64,   162,   179,
     194,   194,   -21,   -21,   -21,   -21,    22,    22,    51,    51,
     -64,   -64,    45,    50,    54,   -64,   -64,    65,    55,   115,
     -64,    64,   -64,    57,   -64,   -64,    49,    40,   -64,   -64,
     -64,    50,   -64,   -64,   -64
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    57,     0,     3,     0,     2,     6,    57,     0,     1,
       0,    14,    49,    38,    39,    39,    31,     0,     0,     0,
      51,     5,    12,    13,    32,    11,     0,     7,    58,     9,
       0,    40,    34,    35,    39,    39,    15,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     8,     0,    36,    37,    16,    18,    17,
      19,    20,    21,    22,    24,    23,    25,    26,    27,    28,
      29,    30,     0,     0,    41,    33,    53,    55,     0,     0,
      50,     0,    43,    46,    48,    45,     0,     0,    52,    10,
      44,     0,    42,    56,    47
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -64,   -64,   -64,   -64,    89,   -64,   -64,    -4,   -64,   -64,
     -13,   -63,     5,   -64,   -64,   -64,    11,    90
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     2,     3,     4,     5,     6,    53,    77,    22,    23,
      32,    75,    85,    24,    25,    26,    78,     8
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      21,     7,    33,    10,    11,    46,    47,    48,    49,    82,
      34,    35,     9,    50,    36,    37,    12,    13,    14,    15,
      16,    55,    56,    92,    17,     1,    -6,    29,    18,    19,
      76,     1,    20,    30,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    10,    11,   -54,    79,
      48,    49,    31,    72,    73,    81,    50,    72,    73,    12,
      13,    14,    15,    16,    74,    70,    71,    17,    74,    83,
      84,    18,    19,    51,    52,    20,    38,    39,    40,    41,
      42,    43,    54,    90,    86,    50,    80,    91,    88,    44,
      45,    46,    47,    48,    49,    27,    94,    28,    93,    50,
       0,    87,    38,    39,    40,    41,    42,    43,     0,     0,
       0,     0,     0,     0,     0,    44,    45,    46,    47,    48,
      49,     0,     0,     0,    57,    50,    38,    39,    40,    41,
      42,    43,    89,     0,     0,     0,     0,     0,     0,    44,
      45,    46,    47,    48,    49,     0,     0,     0,     0,    50,
      38,    39,    40,    41,    42,    43,     0,     0,     0,     0,
       0,     0,     0,    44,    45,    46,    47,    48,    49,     0,
       0,     0,     0,    50,    39,    40,    41,    42,    43,     0,
       0,     0,     0,     0,     0,     0,    44,    45,    46,    47,
      48,    49,    40,    41,    42,    43,    50,     0,     0,     0,
       0,     0,     0,    44,    45,    46,    47,    48,    49,    42,
      43,     0,     0,    50,     0,     0,     0,     0,    44,    45,
      46,    47,    48,    49,     0,     0,     0,     0,    50
};

static const yytype_int8 yycheck[] =
{
       4,    19,    15,     6,     7,    26,    27,    28,    29,    72,
      21,    22,     0,    34,    18,    19,    19,    20,    21,    22,
      23,    34,    35,    86,    27,     5,     0,    17,    31,    32,
      33,     5,    35,    32,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,     6,     7,    32,    53,
      28,    29,    18,     8,     9,    10,    34,     8,     9,    19,
      20,    21,    22,    23,    19,    19,    20,    27,    19,    19,
      20,    31,    32,    30,    32,    35,    11,    12,    13,    14,
      15,    16,    35,    19,    30,    34,    33,    30,    33,    24,
      25,    26,    27,    28,    29,     6,    91,     7,    87,    34,
      -1,    36,    11,    12,    13,    14,    15,    16,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    25,    26,    27,    28,
      29,    -1,    -1,    -1,    33,    34,    11,    12,    13,    14,
      15,    16,    17,    -1,    -1,    -1,    -1,    -1,    -1,    24,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    -1,    34,
      11,    12,    13,    14,    15,    16,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    24,    25,    26,    27,    28,    29,    -1,
      -1,    -1,    -1,    34,    12,    13,    14,    15,    16,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    24,    25,    26,    27,
      28,    29,    13,    14,    15,    16,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    24,    25,    26,    27,    28,    29,    15,
      16,    -1,    -1,    34,    -1,    -1,    -1,    -1,    24,    25,
      26,    27,    28,    29,    -1,    -1,    -1,    -1,    34
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     5,    38,    39,    40,    41,    42,    19,    54,     0,
       6,     7,    19,    20,    21,    22,    23,    27,    31,    32,
      35,    44,    45,    46,    50,    51,    52,    41,    54,    17,
      32,    18,    47,    47,    21,    22,    44,    44,    11,    12,
      13,    14,    15,    16,    24,    25,    26,    27,    28,    29,
      34,    30,    32,    43,    35,    47,    47,    33,    44,    44,
      44,    44,    44,    44,    44,    44,    44,    44,    44,    44,
      19,    20,     8,     9,    19,    48,    33,    44,    53,    44,
      33,    10,    48,    19,    20,    49,    30,    36,    33,    17,
      19,    30,    48,    53,    49
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    37,    38,    38,    40,    39,    41,    41,    42,    43,
      43,    44,    44,    44,    44,    44,    44,    44,    44,    44,
      44,    44,    44,    44,    44,    44,    44,    44,    44,    44,
      44,    44,    44,    44,    45,    45,    45,    45,    46,    47,
      47,    48,    48,    48,    48,    48,    49,    49,    49,    50,
      50,    50,    51,    51,    52,    53,    53,    54,    54
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     0,     2,     0,     2,     4,     0,
       3,     1,     1,     1,     1,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     1,     1,     3,     2,     2,     3,     3,     1,     0,
       1,     1,     3,     2,     3,     2,     1,     3,     1,     1,
       4,     1,     4,     3,     1,     1,     3,     0,     2
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
      yyerror (prg_out, YY_("syntax error: cannot back up")); \
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
                  Type, Value, prg_out); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, pcb_qry_node_t **prg_out)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (prg_out);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, pcb_qry_node_t **prg_out)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, prg_out);
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, pcb_qry_node_t **prg_out)
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
                                              , prg_out);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, prg_out); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, pcb_qry_node_t **prg_out)
{
  YYUSE (yyvaluep);
  YYUSE (prg_out);
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
yyparse (pcb_qry_node_t **prg_out)
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
        case 2:
#line 156 "query_y.y" /* yacc.c:1646  */
    { *prg_out = (yyvsp[0].n); }
#line 1440 "query_y.c" /* yacc.c:1646  */
    break;

  case 3:
#line 157 "query_y.y" /* yacc.c:1646  */
    { *prg_out = (yyvsp[0].n); }
#line 1446 "query_y.c" /* yacc.c:1646  */
    break;

  case 4:
#line 162 "query_y.y" /* yacc.c:1646  */
    { iter_ctx = pcb_qry_iter_alloc(); }
#line 1452 "query_y.c" /* yacc.c:1646  */
    break;

  case 5:
#line 163 "query_y.y" /* yacc.c:1646  */
    {
		(yyval.n) = pcb_qry_n_alloc(PCBQ_EXPR_PROG);
		(yyval.n)->data.children = pcb_qry_n_alloc(PCBQ_ITER_CTX);
		(yyval.n)->data.children->parent = (yyval.n);
		(yyval.n)->data.children->data.iter_ctx = iter_ctx;
		(yyval.n)->data.children->next = (yyvsp[0].n);
		(yyvsp[0].n)->parent = (yyval.n);
	}
#line 1465 "query_y.c" /* yacc.c:1646  */
    break;

  case 6:
#line 175 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = NULL; }
#line 1471 "query_y.c" /* yacc.c:1646  */
    break;

  case 7:
#line 176 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[-1].n); (yyvsp[-1].n)->next = (yyvsp[0].n); }
#line 1477 "query_y.c" /* yacc.c:1646  */
    break;

  case 8:
#line 180 "query_y.y" /* yacc.c:1646  */
    {
		(yyval.n) = pcb_qry_n_alloc(PCBQ_RULE);
		(yyval.n)->data.children = (yyvsp[-2].n);
		(yyvsp[-2].n)->parent = (yyval.n);
		(yyval.n)->data.children->next = pcb_qry_n_alloc(PCBQ_ITER_CTX);
		(yyval.n)->data.children->next->data.iter_ctx = iter_ctx;
		(yyval.n)->data.children->next->next = (yyvsp[0].n);
		(yyvsp[0].n)->parent = (yyval.n);
		}
#line 1491 "query_y.c" /* yacc.c:1646  */
    break;

  case 9:
#line 192 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = NULL; }
#line 1497 "query_y.c" /* yacc.c:1646  */
    break;

  case 10:
#line 193 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[-2].n); (yyvsp[-2].n)->next = (yyvsp[-1].n); }
#line 1503 "query_y.c" /* yacc.c:1646  */
    break;

  case 11:
#line 197 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1509 "query_y.c" /* yacc.c:1646  */
    break;

  case 12:
#line 198 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1515 "query_y.c" /* yacc.c:1646  */
    break;

  case 13:
#line 199 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1521 "query_y.c" /* yacc.c:1646  */
    break;

  case 14:
#line 200 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_INVALID); }
#line 1527 "query_y.c" /* yacc.c:1646  */
    break;

  case 15:
#line 201 "query_y.y" /* yacc.c:1646  */
    { UNOP((yyval.n), PCBQ_OP_NOT, (yyvsp[0].n)); }
#line 1533 "query_y.c" /* yacc.c:1646  */
    break;

  case 16:
#line 202 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[-1].n); }
#line 1539 "query_y.c" /* yacc.c:1646  */
    break;

  case 17:
#line 203 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_AND, (yyvsp[0].n)); }
#line 1545 "query_y.c" /* yacc.c:1646  */
    break;

  case 18:
#line 204 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_OR, (yyvsp[0].n)); }
#line 1551 "query_y.c" /* yacc.c:1646  */
    break;

  case 19:
#line 205 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_EQ, (yyvsp[0].n)); }
#line 1557 "query_y.c" /* yacc.c:1646  */
    break;

  case 20:
#line 206 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_NEQ, (yyvsp[0].n)); }
#line 1563 "query_y.c" /* yacc.c:1646  */
    break;

  case 21:
#line 207 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_GTEQ, (yyvsp[0].n)); }
#line 1569 "query_y.c" /* yacc.c:1646  */
    break;

  case 22:
#line 208 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_LTEQ, (yyvsp[0].n)); }
#line 1575 "query_y.c" /* yacc.c:1646  */
    break;

  case 23:
#line 209 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_GT, (yyvsp[0].n)); }
#line 1581 "query_y.c" /* yacc.c:1646  */
    break;

  case 24:
#line 210 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_LT, (yyvsp[0].n)); }
#line 1587 "query_y.c" /* yacc.c:1646  */
    break;

  case 25:
#line 211 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_ADD, (yyvsp[0].n)); }
#line 1593 "query_y.c" /* yacc.c:1646  */
    break;

  case 26:
#line 212 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_SUB, (yyvsp[0].n)); }
#line 1599 "query_y.c" /* yacc.c:1646  */
    break;

  case 27:
#line 213 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_MUL, (yyvsp[0].n)); }
#line 1605 "query_y.c" /* yacc.c:1646  */
    break;

  case 28:
#line 214 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_DIV, (yyvsp[0].n)); }
#line 1611 "query_y.c" /* yacc.c:1646  */
    break;

  case 29:
#line 215 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_MATCH, make_regex_free((yyvsp[0].s))); }
#line 1617 "query_y.c" /* yacc.c:1646  */
    break;

  case 30:
#line 216 "query_y.y" /* yacc.c:1646  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_MATCH, make_regex_free((yyvsp[0].s))); }
#line 1623 "query_y.c" /* yacc.c:1646  */
    break;

  case 31:
#line 217 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1629 "query_y.c" /* yacc.c:1646  */
    break;

  case 32:
#line 218 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1635 "query_y.c" /* yacc.c:1646  */
    break;

  case 33:
#line 219 "query_y.y" /* yacc.c:1646  */
    {
		pcb_qry_node_t *n;
		(yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD_OF);
		(yyval.n)->data.children = (yyvsp[-2].n);
		(yyvsp[-2].n)->next = (yyvsp[0].n);
		(yyvsp[-2].n)->parent = (yyval.n);
		for(n = (yyvsp[0].n); n != NULL; n = n->next)
			n->parent = (yyval.n);
		}
#line 1649 "query_y.c" /* yacc.c:1646  */
    break;

  case 34:
#line 231 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_COORD);  UNIT_CONV((yyval.n)->data.crd, 0, (yyvsp[-1].c), (yyvsp[0].u)); }
#line 1655 "query_y.c" /* yacc.c:1646  */
    break;

  case 35:
#line 232 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_DOUBLE); UNIT_CONV((yyval.n)->data.dbl, 0, (yyvsp[-1].d), (yyvsp[0].u)); }
#line 1661 "query_y.c" /* yacc.c:1646  */
    break;

  case 36:
#line 233 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_COORD);  UNIT_CONV((yyval.n)->data.crd, 1, (yyvsp[-1].c), (yyvsp[0].u)); }
#line 1667 "query_y.c" /* yacc.c:1646  */
    break;

  case 37:
#line 234 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_DOUBLE); UNIT_CONV((yyval.n)->data.dbl, 1, (yyvsp[-1].d), (yyvsp[0].u)); }
#line 1673 "query_y.c" /* yacc.c:1646  */
    break;

  case 38:
#line 238 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_STRING);  (yyval.n)->data.str = (yyvsp[0].s); }
#line 1679 "query_y.c" /* yacc.c:1646  */
    break;

  case 39:
#line 242 "query_y.y" /* yacc.c:1646  */
    { (yyval.u) = NULL; }
#line 1685 "query_y.c" /* yacc.c:1646  */
    break;

  case 40:
#line 243 "query_y.y" /* yacc.c:1646  */
    { (yyval.u) = (yyvsp[0].u); }
#line 1691 "query_y.c" /* yacc.c:1646  */
    break;

  case 41:
#line 247 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = (yyvsp[0].s); (yyval.n)->precomp.fld = query_fields_sphash((yyvsp[0].s)); }
#line 1697 "query_y.c" /* yacc.c:1646  */
    break;

  case 42:
#line 248 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = (yyvsp[-2].s); (yyval.n)->precomp.fld = query_fields_sphash((yyvsp[-2].s)); (yyval.n)->next = (yyvsp[0].n); }
#line 1703 "query_y.c" /* yacc.c:1646  */
    break;

  case 43:
#line 249 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[0].n); /* just ignore .p. */ }
#line 1709 "query_y.c" /* yacc.c:1646  */
    break;

  case 44:
#line 250 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = make_flag_free((yyvsp[0].s)); }
#line 1715 "query_y.c" /* yacc.c:1646  */
    break;

  case 45:
#line 251 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = pcb_strdup("a"); (yyval.n)->precomp.fld = query_fields_sphash("a"); (yyval.n)->next = (yyvsp[0].n); }
#line 1721 "query_y.c" /* yacc.c:1646  */
    break;

  case 46:
#line 255 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = (yyvsp[0].s); }
#line 1727 "query_y.c" /* yacc.c:1646  */
    break;

  case 47:
#line 256 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = attrib_prepend_free((char *)(yyvsp[0].n)->data.str, (yyvsp[-2].s), '.'); }
#line 1733 "query_y.c" /* yacc.c:1646  */
    break;

  case 48:
#line 257 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = (yyvsp[0].s); }
#line 1739 "query_y.c" /* yacc.c:1646  */
    break;

  case 49:
#line 261 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_VAR); (yyval.n)->data.crd = pcb_qry_iter_var(iter_ctx, (yyvsp[0].s), 1); free((yyvsp[0].s)); }
#line 1745 "query_y.c" /* yacc.c:1646  */
    break;

  case 50:
#line 262 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_LISTVAR); (yyval.n)->data.str = pcb_strdup("@"); }
#line 1751 "query_y.c" /* yacc.c:1646  */
    break;

  case 51:
#line 263 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_VAR); (yyval.n)->data.crd = pcb_qry_iter_var(iter_ctx, "@", 1); }
#line 1757 "query_y.c" /* yacc.c:1646  */
    break;

  case 52:
#line 267 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FCALL); (yyval.n)->data.children = (yyvsp[-3].n); (yyval.n)->data.children->next = (yyvsp[-1].n); (yyvsp[-3].n)->parent = (yyvsp[-1].n)->parent = (yyval.n); }
#line 1763 "query_y.c" /* yacc.c:1646  */
    break;

  case 53:
#line 268 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FCALL); (yyval.n)->data.children = (yyvsp[-2].n); (yyvsp[-2].n)->parent = (yyval.n); }
#line 1769 "query_y.c" /* yacc.c:1646  */
    break;

  case 54:
#line 272 "query_y.y" /* yacc.c:1646  */
    {
		(yyval.n) = pcb_qry_n_alloc(PCBQ_FNAME);
		(yyval.n)->data.fnc = pcb_qry_fnc_lookup((yyvsp[0].s));
		if ((yyval.n)->data.fnc == NULL) {
			yyerror("Unknown function");
			free((yyvsp[0].s));
			return -1;
		}
		free((yyvsp[0].s));
	}
#line 1784 "query_y.c" /* yacc.c:1646  */
    break;

  case 55:
#line 286 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1790 "query_y.c" /* yacc.c:1646  */
    break;

  case 56:
#line 287 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = (yyvsp[-2].n); (yyval.n)->next = (yyvsp[0].n); }
#line 1796 "query_y.c" /* yacc.c:1646  */
    break;

  case 57:
#line 291 "query_y.y" /* yacc.c:1646  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_RNAME); (yyval.n)->data.str = (const char *)pcb_strdup(""); }
#line 1802 "query_y.c" /* yacc.c:1646  */
    break;

  case 58:
#line 292 "query_y.y" /* yacc.c:1646  */
    {
			int l1 = strlen((yyvsp[0].n)->data.str), l2 = strlen((yyvsp[-1].s));
			
			(yyvsp[0].n)->data.str = (const char *)realloc((void *)(yyvsp[0].n)->data.str, l1+l2+2);
			memcpy((char *)(yyvsp[0].n)->data.str+l1, (yyvsp[-1].s), l2+1);
			free((yyvsp[-1].s));
		}
#line 1814 "query_y.c" /* yacc.c:1646  */
    break;


#line 1818 "query_y.c" /* yacc.c:1646  */
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
      yyerror (prg_out, YY_("syntax error"));
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
        yyerror (prg_out, yymsgp);
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
                      yytoken, &yylval, prg_out);
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
                  yystos[yystate], yyvsp, prg_out);
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
  yyerror (prg_out, YY_("memory exhausted"));
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
                  yytoken, &yylval, prg_out);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, prg_out);
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
