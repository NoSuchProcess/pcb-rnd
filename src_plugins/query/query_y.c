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
#define yyparse         qry_parse
#define yylex           qry_lex
#define yyerror         qry_error
#define yydebug         qry_debug
#define yynerrs         qry_nerrs

#define yylval          qry_lval
#define yychar          qry_char

/* First part of user prologue.  */
#line 1 "query_y.y" /* yacc.c:337  */

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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Query language - compiler: grammar */

#include <assert.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include <librnd/core/unit.h>
#include "query.h"
#include "query_l.h"
#include <librnd/core/compat_misc.h>
#include "flag_str.h"
#include "fields_sphash.h"

#define UNIT_CONV(dst, negative, val, unit) \
do { \
	dst = val; \
	if (negative) \
		dst = -dst; \
	if (unit != NULL) { \
		if (unit->family == RND_UNIT_IMPERIAL) \
			dst = RND_MIL_TO_COORD(dst); \
		else if (unit->family == RND_UNIT_METRIC) \
			dst = RND_MM_TO_COORD(dst); \
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
static vti0_t *iter_active_ctx;
static htsp_t *user_funcs;

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

static void link_user_funcs_(pcb_qry_node_t *root, int allow)
{
	pcb_qry_node_t *n, *f, *fname;

	for(n = root; n != NULL; n = n->next) {
		if (pcb_qry_nodetype_has_children(n->type))
			link_user_funcs_(n->data.children, allow);
		if (n->type == PCBQ_FCALL) {
			fname = n->data.children;
			if (fname->precomp.fnc.bui != NULL) /* builtin */
				continue;

			if (user_funcs != NULL)
				f = htsp_get(user_funcs, fname->data.str);
			else
				f = NULL;

			fname->precomp.fnc.uf = f;
			if (f == NULL) {
				yyerror(NULL, "user function not defined:");
				yyerror(NULL, fname->data.str);
			}
		}
	}

}

static void link_user_funcs(pcb_qry_node_t *root, int allow)
{
	link_user_funcs_(root, allow);

	if (user_funcs != NULL)
		htsp_free(user_funcs);
	user_funcs = NULL;
}



#line 231 "query_y.c" /* yacc.c:337  */
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
    T_FUNCTION = 266,
    T_RETURN = 267,
    T_OR = 268,
    T_AND = 269,
    T_EQ = 270,
    T_NEQ = 271,
    T_GTEQ = 272,
    T_LTEQ = 273,
    T_NL = 274,
    T_UNIT = 275,
    T_STR = 276,
    T_QSTR = 277,
    T_INT = 278,
    T_DBL = 279,
    T_CONST = 280,
    T_THUS = 281
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 158 "query_y.y" /* yacc.c:352  */

	char *s;
	rnd_coord_t c;
	double d;
	const rnd_unit_t *u;
	pcb_qry_node_t *n;

#line 309 "query_y.c" /* yacc.c:352  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE qry_lval;

int qry_parse (pcb_qry_node_t **prg_out);

#endif /* !YY_QRY_QUERY_Y_H_INCLUDED  */



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
#define YYFINAL  14
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   249

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  41
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  32
/* YYNRULES -- Number of rules.  */
#define YYNRULES  80
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  134

#define YYUNDEFTOK  2
#define YYMAXUTOK   281

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
       2,     2,     2,    34,     2,     2,    39,     2,     2,     2,
      35,    36,    31,    29,    40,    30,    33,    32,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      27,     2,    28,     2,    38,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,    37,     2,     2,     2,
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
      25,    26
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   199,   199,   200,   205,   205,   218,   219,   223,   231,
     232,   244,   245,   246,   247,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     287,   288,   289,   290,   294,   298,   299,   303,   304,   305,
     306,   307,   311,   312,   313,   318,   317,   337,   336,   351,
     350,   364,   365,   366,   367,   372,   392,   393,   397,   412,
     413,   418,   425,   426,   430,   437,   438,   443,   442,   464,
     465
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_LET", "T_ASSERT", "T_RULE", "T_LIST",
  "T_INVALID", "T_FLD_P", "T_FLD_A", "T_FLD_FLAG", "T_FUNCTION",
  "T_RETURN", "T_OR", "T_AND", "T_EQ", "T_NEQ", "T_GTEQ", "T_LTEQ", "T_NL",
  "T_UNIT", "T_STR", "T_QSTR", "T_INT", "T_DBL", "T_CONST", "T_THUS",
  "'<'", "'>'", "'+'", "'-'", "'*'", "'/'", "'.'", "'!'", "'('", "')'",
  "'~'", "'@'", "'$'", "','", "$accept", "program", "program_expr", "$@1",
  "program_rules", "rule", "rule_or_fdef", "rule_item", "expr", "number",
  "string_literal", "maybe_unit", "fields", "attribs", "let", "$@2",
  "assert", "$@3", "freturn", "$@4", "var", "constant", "fcall",
  "fcallname", "fargs", "fdefarg", "fdefargs", "fdefname", "fdef_", "fdef",
  "$@5", "words", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,    60,    62,    43,
      45,    42,    47,    46,    33,    40,    41,   126,    64,    36,
      44
};
# endif

#define YYPACT_NINF -90

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-90)))

#define YYTABLE_NINF -69

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       4,   -11,    -7,    16,   -90,    34,   -90,     2,    13,   -90,
     -11,   -90,   -90,   -90,   -90,     1,   -90,    28,   -90,    45,
      45,   -90,    85,    34,    34,   -90,    46,   114,   -90,   -90,
     -90,   -90,   -90,    75,   -90,    27,   -90,    88,   -10,   -90,
     -90,   -90,    45,    45,    87,    89,   -90,    34,    34,    34,
      34,    34,    34,    34,    34,    34,    34,    34,    34,    34,
      62,    78,    -1,   -90,   -90,   -90,   -90,    94,   115,   116,
     -19,   -90,    97,   100,   -90,   -90,   -90,   163,   186,   203,
     203,   212,   212,   139,   212,   212,    29,    29,   -25,   -25,
      76,    90,   104,   -90,   -90,   -90,   -90,    61,   102,   118,
      34,    34,    27,    27,    27,   -90,   -90,   108,   113,   -90,
     -90,   129,   -90,   125,   -90,   -90,    62,    34,   -90,    34,
     114,   114,   -90,   -90,   -90,   138,   -90,   -90,    90,   -90,
     -90,   114,   -90,   -90
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    79,     0,     0,     3,     0,     2,     6,     0,     9,
      79,    10,    74,    77,     1,     0,    18,    61,    44,    45,
      45,    36,     0,     0,     0,    64,     0,     5,    16,    17,
      37,    38,    15,     0,     7,    11,    80,     0,     0,    46,
      40,    41,    45,    45,    19,     0,    65,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    55,    57,    59,     8,     0,     0,     0,
       0,    78,     0,     0,    42,    43,    20,    23,    22,    24,
      25,    26,    27,    21,    29,    28,    30,    31,    32,    33,
       0,     0,    47,    39,    34,    35,    67,    69,     0,     0,
       0,     0,    11,    11,    11,    71,    76,    72,     0,    63,
      62,     0,    49,    52,    54,    51,     0,     0,    66,     0,
      58,    60,    13,    12,    14,     0,    75,    50,     0,    48,
      70,    56,    73,    53
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -90,   -90,   -90,   -90,   153,   -90,   -90,   -22,    -5,   -90,
     -90,   -17,   -89,    33,   -90,   -90,   -90,   -90,   -90,   -90,
     -90,   -90,   -90,   -90,    47,    37,   -90,   -90,   -90,   -90,
     -90,   155
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     3,     4,     5,     6,     7,     8,    66,    97,    28,
      29,    40,    93,   115,    67,    99,    68,   100,    69,   101,
      30,    31,    32,    33,    98,   107,   108,    13,    71,     9,
      37,    11
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      27,   112,   105,    41,    -6,    15,    16,     1,    60,     1,
      10,    72,    61,     2,    12,     2,    14,   106,    44,    45,
      17,    18,    19,    20,    21,    74,    75,   129,    73,    22,
      63,    64,    35,    23,    24,    96,    38,    25,    26,    65,
      15,    16,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    17,    18,    19,    20,    21,
      58,    59,    60,   -68,    22,    39,    61,    46,    23,    24,
      90,    91,    25,    26,    47,    48,    49,    50,    51,    52,
     122,   123,   124,    92,    90,    91,   111,    53,    54,    55,
      56,    57,    58,    59,    60,   120,   121,    92,    61,    94,
      95,   117,    47,    48,    49,    50,    51,    52,    42,    43,
      62,   113,   114,   102,   131,    53,    54,    55,    56,    57,
      58,    59,    60,    70,    61,    76,    61,    47,    48,    49,
      50,    51,    52,   109,   103,   104,   110,   116,   118,   119,
      53,    54,    55,    56,    57,    58,    59,    60,   125,   126,
     127,    61,    47,    48,    49,    50,    51,    52,   128,   105,
      34,   133,   132,     0,   130,    36,    54,    55,    56,    57,
      58,    59,    60,     0,     0,     0,    61,    48,    49,    50,
      51,    52,     0,     0,     0,     0,     0,     0,     0,     0,
      54,    55,    56,    57,    58,    59,    60,     0,     0,     0,
      61,    49,    50,    51,    52,     0,     0,     0,     0,     0,
       0,     0,     0,    54,    55,    56,    57,    58,    59,    60,
      51,    52,     0,    61,     0,     0,     0,     0,     0,     0,
      54,    55,    56,    57,    58,    59,    60,     0,     0,     0,
      61,    56,    57,    58,    59,    60,     0,     0,     0,    61
};

static const yytype_int16 yycheck[] =
{
       5,    90,    21,    20,     0,     6,     7,     5,    33,     5,
      21,    21,    37,    11,    21,    11,     0,    36,    23,    24,
      21,    22,    23,    24,    25,    42,    43,   116,    38,    30,
       3,     4,    19,    34,    35,    36,    35,    38,    39,    12,
       6,     7,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    21,    22,    23,    24,    25,
      31,    32,    33,    35,    30,    20,    37,    21,    34,    35,
       8,     9,    38,    39,    13,    14,    15,    16,    17,    18,
     102,   103,   104,    21,     8,     9,    10,    26,    27,    28,
      29,    30,    31,    32,    33,   100,   101,    21,    37,    21,
      22,    40,    13,    14,    15,    16,    17,    18,    23,    24,
      35,    21,    22,    19,   119,    26,    27,    28,    29,    30,
      31,    32,    33,    35,    37,    36,    37,    13,    14,    15,
      16,    17,    18,    36,    19,    19,    36,    33,    36,    21,
      26,    27,    28,    29,    30,    31,    32,    33,    40,    36,
      21,    37,    13,    14,    15,    16,    17,    18,    33,    21,
       7,   128,   125,    -1,   117,    10,    27,    28,    29,    30,
      31,    32,    33,    -1,    -1,    -1,    37,    14,    15,    16,
      17,    18,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      27,    28,    29,    30,    31,    32,    33,    -1,    -1,    -1,
      37,    15,    16,    17,    18,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    27,    28,    29,    30,    31,    32,    33,
      17,    18,    -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,
      27,    28,    29,    30,    31,    32,    33,    -1,    -1,    -1,
      37,    29,    30,    31,    32,    33,    -1,    -1,    -1,    37
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     5,    11,    42,    43,    44,    45,    46,    47,    70,
      21,    72,    21,    68,     0,     6,     7,    21,    22,    23,
      24,    25,    30,    34,    35,    38,    39,    49,    50,    51,
      61,    62,    63,    64,    45,    19,    72,    71,    35,    20,
      52,    52,    23,    24,    49,    49,    21,    13,    14,    15,
      16,    17,    18,    26,    27,    28,    29,    30,    31,    32,
      33,    37,    35,     3,     4,    12,    48,    55,    57,    59,
      35,    69,    21,    38,    52,    52,    36,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
       8,     9,    21,    53,    21,    22,    36,    49,    65,    56,
      58,    60,    19,    19,    19,    21,    36,    66,    67,    36,
      36,    10,    53,    21,    22,    54,    33,    40,    36,    21,
      49,    49,    48,    48,    48,    40,    36,    21,    33,    53,
      65,    49,    66,    54
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    41,    42,    42,    44,    43,    45,    45,    46,    47,
      47,    48,    48,    48,    48,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      50,    50,    50,    50,    51,    52,    52,    53,    53,    53,
      53,    53,    54,    54,    54,    56,    55,    58,    57,    60,
      59,    61,    61,    61,    61,    62,    63,    63,    64,    65,
      65,    66,    67,    67,    68,    69,    69,    71,    70,    72,
      72
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     0,     2,     0,     2,     3,     1,
       2,     0,     3,     3,     3,     1,     1,     1,     1,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     1,     3,
       2,     2,     3,     3,     1,     0,     1,     1,     3,     2,
       3,     2,     1,     3,     1,     0,     4,     0,     3,     0,
       3,     1,     4,     4,     1,     2,     4,     3,     1,     1,
       3,     1,     1,     3,     1,     3,     2,     0,     4,     0,
       2
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


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, pcb_qry_node_t **prg_out)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (prg_out);
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
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, pcb_qry_node_t **prg_out)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep, prg_out);
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, pcb_qry_node_t **prg_out)
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
        case 2:
#line 199 "query_y.y" /* yacc.c:1652  */
    { *prg_out = (yyvsp[0].n); link_user_funcs((yyvsp[0].n), 1); }
#line 1524 "query_y.c" /* yacc.c:1652  */
    break;

  case 3:
#line 200 "query_y.y" /* yacc.c:1652  */
    { *prg_out = (yyvsp[0].n); assert(user_funcs == NULL); link_user_funcs((yyvsp[0].n), 0); }
#line 1530 "query_y.c" /* yacc.c:1652  */
    break;

  case 4:
#line 205 "query_y.y" /* yacc.c:1652  */
    { iter_ctx = pcb_qry_iter_alloc(); }
#line 1536 "query_y.c" /* yacc.c:1652  */
    break;

  case 5:
#line 206 "query_y.y" /* yacc.c:1652  */
    {
		(yyval.n) = pcb_qry_n_alloc(PCBQ_EXPR_PROG);
		(yyval.n)->data.children = pcb_qry_n_alloc(PCBQ_ITER_CTX);
		(yyval.n)->data.children->parent = (yyval.n);
		(yyval.n)->data.children->data.iter_ctx = iter_ctx;
		(yyval.n)->data.children->next = (yyvsp[0].n);
		(yyvsp[0].n)->parent = (yyval.n);
	}
#line 1549 "query_y.c" /* yacc.c:1652  */
    break;

  case 6:
#line 218 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = NULL; }
#line 1555 "query_y.c" /* yacc.c:1652  */
    break;

  case 7:
#line 219 "query_y.y" /* yacc.c:1652  */
    { if ((yyvsp[-1].n) != NULL) { (yyval.n) = (yyvsp[-1].n); (yyvsp[-1].n)->next = (yyvsp[0].n); } else { (yyval.n) = (yyvsp[0].n); } }
#line 1561 "query_y.c" /* yacc.c:1652  */
    break;

  case 8:
#line 223 "query_y.y" /* yacc.c:1652  */
    {
		(yyval.n) = (yyvsp[-2].n);
		if ((yyvsp[0].n) != NULL)
			pcb_qry_n_append((yyval.n), (yyvsp[0].n));
	}
#line 1571 "query_y.c" /* yacc.c:1652  */
    break;

  case 9:
#line 231 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1577 "query_y.c" /* yacc.c:1652  */
    break;

  case 10:
#line 232 "query_y.y" /* yacc.c:1652  */
    {
			pcb_qry_node_t *nd;
			iter_ctx = pcb_qry_iter_alloc();
			(yyval.n) = pcb_qry_n_alloc(PCBQ_RULE);
			pcb_qry_n_insert((yyval.n), (yyvsp[0].n));
			nd = pcb_qry_n_alloc(PCBQ_ITER_CTX);
			nd->data.iter_ctx = iter_ctx;
			pcb_qry_n_insert((yyval.n), nd);
	}
#line 1591 "query_y.c" /* yacc.c:1652  */
    break;

  case 11:
#line 244 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = NULL; }
#line 1597 "query_y.c" /* yacc.c:1652  */
    break;

  case 12:
#line 245 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[-2].n); (yyvsp[-2].n)->next = (yyvsp[0].n); }
#line 1603 "query_y.c" /* yacc.c:1652  */
    break;

  case 13:
#line 246 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[-2].n); (yyvsp[-2].n)->next = (yyvsp[0].n); }
#line 1609 "query_y.c" /* yacc.c:1652  */
    break;

  case 14:
#line 247 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[-2].n); (yyvsp[-2].n)->next = (yyvsp[0].n); }
#line 1615 "query_y.c" /* yacc.c:1652  */
    break;

  case 15:
#line 251 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1621 "query_y.c" /* yacc.c:1652  */
    break;

  case 16:
#line 252 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1627 "query_y.c" /* yacc.c:1652  */
    break;

  case 17:
#line 253 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1633 "query_y.c" /* yacc.c:1652  */
    break;

  case 18:
#line 254 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_INVALID); }
#line 1639 "query_y.c" /* yacc.c:1652  */
    break;

  case 19:
#line 255 "query_y.y" /* yacc.c:1652  */
    { UNOP((yyval.n), PCBQ_OP_NOT, (yyvsp[0].n)); }
#line 1645 "query_y.c" /* yacc.c:1652  */
    break;

  case 20:
#line 256 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[-1].n); }
#line 1651 "query_y.c" /* yacc.c:1652  */
    break;

  case 21:
#line 257 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_THUS, (yyvsp[0].n)); }
#line 1657 "query_y.c" /* yacc.c:1652  */
    break;

  case 22:
#line 258 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_AND, (yyvsp[0].n)); }
#line 1663 "query_y.c" /* yacc.c:1652  */
    break;

  case 23:
#line 259 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_OR, (yyvsp[0].n)); }
#line 1669 "query_y.c" /* yacc.c:1652  */
    break;

  case 24:
#line 260 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_EQ, (yyvsp[0].n)); }
#line 1675 "query_y.c" /* yacc.c:1652  */
    break;

  case 25:
#line 261 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_NEQ, (yyvsp[0].n)); }
#line 1681 "query_y.c" /* yacc.c:1652  */
    break;

  case 26:
#line 262 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_GTEQ, (yyvsp[0].n)); }
#line 1687 "query_y.c" /* yacc.c:1652  */
    break;

  case 27:
#line 263 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_LTEQ, (yyvsp[0].n)); }
#line 1693 "query_y.c" /* yacc.c:1652  */
    break;

  case 28:
#line 264 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_GT, (yyvsp[0].n)); }
#line 1699 "query_y.c" /* yacc.c:1652  */
    break;

  case 29:
#line 265 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_LT, (yyvsp[0].n)); }
#line 1705 "query_y.c" /* yacc.c:1652  */
    break;

  case 30:
#line 266 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_ADD, (yyvsp[0].n)); }
#line 1711 "query_y.c" /* yacc.c:1652  */
    break;

  case 31:
#line 267 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_SUB, (yyvsp[0].n)); }
#line 1717 "query_y.c" /* yacc.c:1652  */
    break;

  case 32:
#line 268 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_MUL, (yyvsp[0].n)); }
#line 1723 "query_y.c" /* yacc.c:1652  */
    break;

  case 33:
#line 269 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_DIV, (yyvsp[0].n)); }
#line 1729 "query_y.c" /* yacc.c:1652  */
    break;

  case 34:
#line 270 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_MATCH, make_regex_free((yyvsp[0].s))); }
#line 1735 "query_y.c" /* yacc.c:1652  */
    break;

  case 35:
#line 271 "query_y.y" /* yacc.c:1652  */
    { BINOP((yyval.n), (yyvsp[-2].n), PCBQ_OP_MATCH, make_regex_free((yyvsp[0].s))); }
#line 1741 "query_y.c" /* yacc.c:1652  */
    break;

  case 36:
#line 272 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1747 "query_y.c" /* yacc.c:1652  */
    break;

  case 37:
#line 273 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1753 "query_y.c" /* yacc.c:1652  */
    break;

  case 38:
#line 274 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[0].n); }
#line 1759 "query_y.c" /* yacc.c:1652  */
    break;

  case 39:
#line 275 "query_y.y" /* yacc.c:1652  */
    {
		pcb_qry_node_t *n;
		(yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD_OF);
		(yyval.n)->data.children = (yyvsp[-2].n);
		(yyvsp[-2].n)->next = (yyvsp[0].n);
		(yyvsp[-2].n)->parent = (yyval.n);
		for(n = (yyvsp[0].n); n != NULL; n = n->next)
			n->parent = (yyval.n);
		}
#line 1773 "query_y.c" /* yacc.c:1652  */
    break;

  case 40:
#line 287 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_COORD);  UNIT_CONV((yyval.n)->data.crd, 0, (yyvsp[-1].c), (yyvsp[0].u)); }
#line 1779 "query_y.c" /* yacc.c:1652  */
    break;

  case 41:
#line 288 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_DOUBLE); UNIT_CONV((yyval.n)->data.dbl, 0, (yyvsp[-1].d), (yyvsp[0].u)); }
#line 1785 "query_y.c" /* yacc.c:1652  */
    break;

  case 42:
#line 289 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_COORD);  UNIT_CONV((yyval.n)->data.crd, 1, (yyvsp[-1].c), (yyvsp[0].u)); }
#line 1791 "query_y.c" /* yacc.c:1652  */
    break;

  case 43:
#line 290 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_DOUBLE); UNIT_CONV((yyval.n)->data.dbl, 1, (yyvsp[-1].d), (yyvsp[0].u)); }
#line 1797 "query_y.c" /* yacc.c:1652  */
    break;

  case 44:
#line 294 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_DATA_STRING);  (yyval.n)->data.str = (yyvsp[0].s); }
#line 1803 "query_y.c" /* yacc.c:1652  */
    break;

  case 45:
#line 298 "query_y.y" /* yacc.c:1652  */
    { (yyval.u) = NULL; }
#line 1809 "query_y.c" /* yacc.c:1652  */
    break;

  case 46:
#line 299 "query_y.y" /* yacc.c:1652  */
    { (yyval.u) = (yyvsp[0].u); }
#line 1815 "query_y.c" /* yacc.c:1652  */
    break;

  case 47:
#line 303 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = (yyvsp[0].s); (yyval.n)->precomp.fld = query_fields_sphash((yyvsp[0].s)); }
#line 1821 "query_y.c" /* yacc.c:1652  */
    break;

  case 48:
#line 304 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = (yyvsp[-2].s); (yyval.n)->precomp.fld = query_fields_sphash((yyvsp[-2].s)); (yyval.n)->next = (yyvsp[0].n); }
#line 1827 "query_y.c" /* yacc.c:1652  */
    break;

  case 49:
#line 305 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[0].n); /* just ignore .p. */ }
#line 1833 "query_y.c" /* yacc.c:1652  */
    break;

  case 50:
#line 306 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = make_flag_free((yyvsp[0].s)); }
#line 1839 "query_y.c" /* yacc.c:1652  */
    break;

  case 51:
#line 307 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = rnd_strdup("a"); (yyval.n)->precomp.fld = query_fields_sphash("a"); (yyval.n)->next = (yyvsp[0].n); }
#line 1845 "query_y.c" /* yacc.c:1652  */
    break;

  case 52:
#line 311 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = (yyvsp[0].s); }
#line 1851 "query_y.c" /* yacc.c:1652  */
    break;

  case 53:
#line 312 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = attrib_prepend_free((char *)(yyvsp[0].n)->data.str, (yyvsp[-2].s), '.'); }
#line 1857 "query_y.c" /* yacc.c:1652  */
    break;

  case 54:
#line 313 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FIELD); (yyval.n)->data.str = (yyvsp[0].s); }
#line 1863 "query_y.c" /* yacc.c:1652  */
    break;

  case 55:
#line 318 "query_y.y" /* yacc.c:1652  */
    {
				iter_active_ctx = calloc(sizeof(vti0_t), 1);
			}
#line 1871 "query_y.c" /* yacc.c:1652  */
    break;

  case 56:
#line 322 "query_y.y" /* yacc.c:1652  */
    {
				pcb_qry_node_t *nd;
				(yyval.n) = pcb_qry_n_alloc(PCBQ_LET);
				(yyval.n)->precomp.it_active = iter_active_ctx;
				iter_active_ctx = NULL;
				pcb_qry_n_insert((yyval.n), (yyvsp[0].n));
				nd = pcb_qry_n_alloc(PCBQ_VAR);
				nd->data.crd = pcb_qry_iter_var(iter_ctx, (yyvsp[-1].s), 1);
				pcb_qry_n_insert((yyval.n), nd);
				free((yyvsp[-1].s));
			}
#line 1887 "query_y.c" /* yacc.c:1652  */
    break;

  case 57:
#line 337 "query_y.y" /* yacc.c:1652  */
    {
			iter_active_ctx = calloc(sizeof(vti0_t), 1);
		}
#line 1895 "query_y.c" /* yacc.c:1652  */
    break;

  case 58:
#line 341 "query_y.y" /* yacc.c:1652  */
    {
			(yyval.n) = pcb_qry_n_alloc(PCBQ_ASSERT);
			(yyval.n)->precomp.it_active = iter_active_ctx;
			iter_active_ctx = NULL;
			pcb_qry_n_insert((yyval.n), (yyvsp[0].n));
		}
#line 1906 "query_y.c" /* yacc.c:1652  */
    break;

  case 59:
#line 351 "query_y.y" /* yacc.c:1652  */
    {
			iter_active_ctx = calloc(sizeof(vti0_t), 1);
		}
#line 1914 "query_y.c" /* yacc.c:1652  */
    break;

  case 60:
#line 355 "query_y.y" /* yacc.c:1652  */
    {
			(yyval.n) = pcb_qry_n_alloc(PCBQ_RETURN);
			(yyval.n)->precomp.it_active = iter_active_ctx;
			iter_active_ctx = NULL;
			pcb_qry_n_insert((yyval.n), (yyvsp[0].n));
		}
#line 1925 "query_y.c" /* yacc.c:1652  */
    break;

  case 61:
#line 364 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_VAR); (yyval.n)->data.crd = pcb_qry_iter_var(iter_ctx, (yyvsp[0].s), 1); if (iter_active_ctx != NULL) vti0_set(iter_active_ctx, (yyval.n)->data.crd, 1); free((yyvsp[0].s)); }
#line 1931 "query_y.c" /* yacc.c:1652  */
    break;

  case 62:
#line 365 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_LISTVAR); (yyval.n)->data.str = rnd_strdup("@"); /* delibertely not setting iter_active, list() protects against turning it into an iterator */ }
#line 1937 "query_y.c" /* yacc.c:1652  */
    break;

  case 63:
#line 366 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_LISTVAR); (yyval.n)->data.str = (yyvsp[-1].s); /* delibertely not setting iter_active, list() protects against turning it into an iterator */ }
#line 1943 "query_y.c" /* yacc.c:1652  */
    break;

  case 64:
#line 367 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_VAR); (yyval.n)->data.crd = pcb_qry_iter_var(iter_ctx, "@", 1); if (iter_active_ctx != NULL) vti0_set(iter_active_ctx, (yyval.n)->data.crd, 1); }
#line 1949 "query_y.c" /* yacc.c:1652  */
    break;

  case 65:
#line 373 "query_y.y" /* yacc.c:1652  */
    {
			pcb_qry_node_t *fname, *nname;

			nname = pcb_qry_n_alloc(PCBQ_DATA_STRING);
			nname->data.str = rnd_concat("design/drc/", (yyvsp[0].s), NULL);
			free((yyvsp[0].s));

			fname = pcb_qry_n_alloc(PCBQ_FNAME);
			fname->data.str = NULL;
			fname->precomp.fnc.bui = pcb_qry_fnc_lookup("getconf");

			(yyval.n) = pcb_qry_n_alloc(PCBQ_FCALL);
			fname->parent = nname->parent = (yyval.n);
			(yyval.n)->data.children = fname;
			(yyval.n)->data.children->next = nname;
		}
#line 1970 "query_y.c" /* yacc.c:1652  */
    break;

  case 66:
#line 392 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FCALL); (yyval.n)->data.children = (yyvsp[-3].n); (yyval.n)->data.children->next = (yyvsp[-1].n); (yyvsp[-3].n)->parent = (yyvsp[-1].n)->parent = (yyval.n); }
#line 1976 "query_y.c" /* yacc.c:1652  */
    break;

  case 67:
#line 393 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_FCALL); (yyval.n)->data.children = (yyvsp[-2].n); (yyvsp[-2].n)->parent = (yyval.n); }
#line 1982 "query_y.c" /* yacc.c:1652  */
    break;

  case 68:
#line 397 "query_y.y" /* yacc.c:1652  */
    {
		(yyval.n) = pcb_qry_n_alloc(PCBQ_FNAME);
		(yyval.n)->precomp.fnc.bui = pcb_qry_fnc_lookup((yyvsp[0].s));
		if ((yyval.n)->precomp.fnc.bui != NULL) {
			/* builtin function */
			free((yyvsp[0].s));
			(yyval.n)->data.str = NULL;
		}
		else
			(yyval.n)->data.str = (yyvsp[0].s); /* user function: save the name */
	}
#line 1998 "query_y.c" /* yacc.c:1652  */
    break;

  case 69:
#line 412 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[0].n); }
#line 2004 "query_y.c" /* yacc.c:1652  */
    break;

  case 70:
#line 413 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[-2].n); (yyval.n)->next = (yyvsp[0].n); }
#line 2010 "query_y.c" /* yacc.c:1652  */
    break;

  case 71:
#line 418 "query_y.y" /* yacc.c:1652  */
    {
		(yyval.n) = pcb_qry_n_alloc(PCBQ_ARG);
		(yyval.n)->data.crd = pcb_qry_iter_var(iter_ctx, (yyvsp[0].s), 1);
	}
#line 2019 "query_y.c" /* yacc.c:1652  */
    break;

  case 72:
#line 425 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[0].n); }
#line 2025 "query_y.c" /* yacc.c:1652  */
    break;

  case 73:
#line 426 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[-2].n); (yyval.n)->next = (yyvsp[0].n); }
#line 2031 "query_y.c" /* yacc.c:1652  */
    break;

  case 74:
#line 430 "query_y.y" /* yacc.c:1652  */
    {
		(yyval.n) = pcb_qry_n_alloc(PCBQ_FNAME);
		(yyval.n)->data.str = (yyvsp[0].s);
	}
#line 2040 "query_y.c" /* yacc.c:1652  */
    break;

  case 75:
#line 437 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = (yyvsp[-1].n); }
#line 2046 "query_y.c" /* yacc.c:1652  */
    break;

  case 76:
#line 438 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = NULL; }
#line 2052 "query_y.c" /* yacc.c:1652  */
    break;

  case 77:
#line 443 "query_y.y" /* yacc.c:1652  */
    { iter_ctx = pcb_qry_iter_alloc(); }
#line 2058 "query_y.c" /* yacc.c:1652  */
    break;

  case 78:
#line 444 "query_y.y" /* yacc.c:1652  */
    {
		pcb_qry_node_t *nd;

		(yyval.n) = pcb_qry_n_alloc(PCBQ_FUNCTION);

		if ((yyvsp[0].n) != NULL)
			pcb_qry_n_append((yyval.n), (yyvsp[0].n));

		pcb_qry_n_insert((yyval.n), (yyvsp[-2].n));

		nd = pcb_qry_n_alloc(PCBQ_ITER_CTX);
		nd->data.iter_ctx = iter_ctx;
		pcb_qry_n_insert((yyval.n), nd);

		if (user_funcs == NULL)
			user_funcs = htsp_alloc(strhash, strkeyeq);
		htsp_set(user_funcs, (char *)(yyvsp[-2].n)->data.str, (yyval.n));
	}
#line 2081 "query_y.c" /* yacc.c:1652  */
    break;

  case 79:
#line 464 "query_y.y" /* yacc.c:1652  */
    { (yyval.n) = pcb_qry_n_alloc(PCBQ_RNAME); (yyval.n)->data.str = (const char *)rnd_strdup(""); }
#line 2087 "query_y.c" /* yacc.c:1652  */
    break;

  case 80:
#line 465 "query_y.y" /* yacc.c:1652  */
    {
			char *old = (char *)(yyvsp[0].n)->data.str, *sep = ((*old != '\0') ? " " : "");
			(yyvsp[0].n)->data.str = rnd_concat((yyvsp[-1].s), sep, old, NULL);
			free(old);
			free((yyvsp[-1].s));
			(yyval.n) = (yyvsp[0].n);
		}
#line 2099 "query_y.c" /* yacc.c:1652  */
    break;


#line 2103 "query_y.c" /* yacc.c:1652  */
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
