
#ifdef YY_QUERY_API_VER
#define YY_BYACCIC
#define YY_API_MAJOR 1
#define YY_API_MINOR 0
#endif /*YY_QUERY_API_VER*/

#define pcb_bxl_EMPTY        (-1)
#define pcb_bxl_clearin      (pcb_bxl_chr = pcb_bxl_EMPTY)
#define pcb_bxl_errok        (pcb_bxl_errflag = 0)
#define pcb_bxl_RECOVERING() (pcb_bxl_errflag != 0)
#define pcb_bxl_ENOMEM       (-2)
#define pcb_bxl_EOF          0
#line 16 "../../src_plugins/io_bxl/bxl_gram.c"
#include "../../src_plugins/io_bxl/bxl_gram.h"
static const pcb_bxl_int_t pcb_bxl_lhs[] = {                     -1,
    0,    2,    2,    3,    3,    3,    3,    9,    9,    4,
    4,    1,    1,   10,   10,   11,    5,   12,   12,   13,
   13,   13,    6,   14,   14,   16,   16,   16,   16,   15,
   15,   17,   18,   18,   19,   19,   19,   19,    7,   20,
   20,   21,   21,   21,   21,   22,   23,   23,   24,   24,
   24,   24,   24,   24,   24,   24,   25,   33,   33,   34,
   34,   34,   34,   34,   34,   34,   26,   35,   35,   36,
   36,   36,   36,   36,   27,   37,   37,   38,   38,   38,
   38,   28,   39,   39,   40,   40,   40,   40,   40,   40,
   30,   41,   41,   42,   42,   42,   42,   42,   42,   31,
   43,   43,   44,   44,   44,   44,   44,   44,   44,   44,
   32,   29,   45,   45,   46,   46,   46,    8,
};
static const pcb_bxl_int_t pcb_bxl_len[] = {                      2,
    2,    0,    3,    1,    1,    1,    1,    1,    1,    1,
    2,    0,    1,    1,    1,    1,    3,    0,    4,    2,
    2,    2,   10,    0,    4,    2,    2,    2,    2,    0,
    2,    4,    0,    4,    2,    2,    2,    2,    5,    0,
    3,    1,    6,    6,    6,    6,    0,    3,    1,    1,
    1,    1,    1,    1,    1,    1,    2,    0,    4,    2,
    2,    2,    2,    4,    2,    2,    2,    0,    4,    2,
    4,    2,    2,    3,    2,    0,    4,    2,    4,    4,
    2,    2,    0,    4,    2,    4,    3,    2,    2,    2,
    2,    0,    4,    2,    4,    2,    2,    2,    2,    2,
    0,    4,    2,    4,    2,    2,    2,    2,    2,    2,
    2,    2,    0,    4,    4,    2,    2,    6,
};
static const pcb_bxl_int_t pcb_bxl_defred[] = {                   0,
    0,    0,    0,   13,   11,    0,    0,    0,    0,    1,
    0,    4,    5,    6,    7,    0,    0,    0,    0,    0,
    0,   17,    0,    0,    0,    0,    3,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   42,    0,   20,   21,   22,    0,   26,
    8,    9,   27,   28,   29,    0,    0,    0,    0,    0,
    0,   39,    0,    0,   19,   25,    0,    0,   14,   15,
   16,    0,    0,    0,   41,  118,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   49,   50,   51,   52,   53,   54,   55,   56,
    0,    0,    0,    0,    0,    0,    0,   57,    0,   67,
    0,   75,    0,   82,    0,   91,    0,  100,    0,  111,
  112,   46,    0,   43,   44,   45,    0,   23,   31,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   48,    0,    0,   60,   61,
   62,   63,    0,   65,   66,    0,   72,   70,    0,   73,
    0,    0,   81,   78,    0,    0,    0,   89,   85,    0,
    0,   88,   90,    0,   96,   94,    0,   97,   98,   99,
    0,  108,  103,    0,  110,  107,  105,  106,  109,    0,
    0,  116,  117,    0,    0,    0,    0,    0,    0,   32,
    0,   59,    0,   74,   69,    0,    0,   77,    0,   87,
   84,    0,   93,    0,  102,    0,  114,   35,   36,   37,
   38,    0,   64,   71,   79,   80,   86,   95,  104,  115,
   34,
};
static const pcb_bxl_int_t pcb_bxl_dgoto[] = {                    2,
    3,   10,   11,    4,   12,   13,   14,   15,   53,   71,
   72,   22,   31,   24,  105,   36,  106,  178,  229,   42,
   43,   44,   91,   92,   93,   94,   95,   96,   97,   98,
   99,  100,  108,  137,  110,  143,  112,  148,  114,  155,
  116,  162,  118,  171,  120,  175,
};
static const pcb_bxl_int_t pcb_bxl_sindex[] = {                  -3,
   -3,    0, -252,    0,    0, -250, -246, -241, -232,    0,
   -3,    0,    0,    0,    0,  -13,   -8,   -3, -279, -252,
 -129,    0, -207,   -3, -188, -274,    0, -214, -209, -204,
   15, -180, -245, -245, -245,   29, -190,   23,   43,   45,
   47, -182,   -3,    0, -150,    0,    0,    0,  -13,    0,
    0,    0,    0,    0,    0,   -8,   33, -143, -191, -191,
 -191,    0, -188, -202,    0,    0, -140,   -3,    0,    0,
    0,   76,   77,   79,    0,    0,   -3, -177, -191, -191,
 -191, -142,   85,   87,   90,   91,   98,  108,  109,  109,
 -131,   -3,    0,    0,    0,    0,    0,    0,    0,    0,
  110,  111,  112, -106, -113, -142, -145,    0, -200,    0,
 -238,    0, -235,    0, -229,    0, -257,    0, -278,    0,
    0,    0, -177,    0,    0,    0,  116,    0,    0, -101,
 -102, -100,  -99, -191,  -96, -191,  118, -191,  -94, -191,
  -95,  120,  125, -191,  -89, -191, -191,  126,  -91,  -87,
 -191,  -88,  -86, -245,  133, -191,  -81, -191, -191, -191,
 -191,  136,  -80,  -75, -191, -191,  -74,  -73, -245, -245,
  148, -191,  -70,  -69,  151,    0, -165,   -3,    0,    0,
    0,    0,  149,    0,    0,   85,    0,    0,  150,    0,
 -191,   87,    0,    0,  152,  153,   90,    0,    0,  154,
  -65,    0,    0,   91,    0,    0,  155,    0,    0,    0,
   98,    0,    0,  156,    0,    0,    0,    0,    0,  108,
  157,    0,    0,  109, -191, -191,  -56,  -54,  163,    0,
 -191,    0, -191,    0,    0, -191, -191,    0, -191,    0,
    0, -191,    0, -191,    0, -191,    0,    0,    0,    0,
    0,  116,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
static const pcb_bxl_int_t pcb_bxl_rindex[] = {                   9,
    1,    0,  205,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  196,  197,    0,    0,  205,
    0,    0,    0,    0,  -71,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  196,    0,
    0,    0,    0,    0,    0,  197,    0,    0,    0,    0,
    0,    0,  -71,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -72,    0,    0,
    0,  -57,  200,  202,  203,  204,  206,  207,  208,  208,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  -57,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  -72,    0,    0,    0,  209,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  200,    0,    0,    0,    0,
    0,  202,    0,    0,    0,    0,  203,    0,    0,    0,
    0,    0,    0,  204,    0,    0,    0,    0,    0,    0,
  206,    0,    0,    0,    0,    0,    0,    0,    0,  207,
    0,    0,    0,  208,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  209,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
static const pcb_bxl_int_t pcb_bxl_gindex[] = {                   0,
    0,  195,    0,    7,    0,    0,    0,    0,  -30, -123,
  -58,  171,    0,  165,  117,    0,    0,  -28,    0,  159,
    0,    0,  102,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   40,    0,   35,    0,   31,    0,   25,    0,
   19,    0,   11,    0,  -90,    0,
};
#define pcb_bxl_TABLESIZE 321
static const pcb_bxl_int_t pcb_bxl_table[] = {                  121,
   10,   73,   74,   54,   55,  163,    1,    5,   12,   16,
    6,  172,  185,   17,    7,   51,   52,   20,   18,  164,
  101,  102,  103,   19,   25,    8,   21,  149,  173,  174,
   37,   23,  165,   26,  166,  144,  209,  210,  145,   45,
  167,  150,  215,   46,  156,  168,  169,  157,   47,   63,
  142,  146,  170,   48,  151,   49,  147,   69,   70,    9,
  158,  152,  153,   32,   33,   34,   69,   70,  154,   56,
  159,  160,  161,  138,   78,  183,  139,   50,   57,  187,
   58,  189,   59,   82,   60,  193,   61,  195,  196,  140,
   67,   38,  200,   39,   40,   41,   62,  205,  123,  207,
  208,  248,  249,   35,  141,   64,  214,   83,  225,  226,
  227,  228,   76,  221,   68,   84,   85,   77,   86,   79,
   80,   87,   81,  203,  107,   88,  109,  104,   89,  111,
  113,   90,  234,  247,   28,   29,   30,  115,  218,  219,
  130,  131,  132,  133,  134,  135,  136,  117,  119,  122,
  124,  125,  126,  127,  128,  177,  179,  180,  186,  181,
  182,  184,  188,  191,  190,  192,  197,  194,  198,  199,
  202,  201,  253,  204,  254,  206,  211,  255,  256,  212,
  257,  213,  216,  258,  230,  259,  217,  260,  220,  222,
  223,  224,  231,  233,  240,  236,  237,  239,  242,  244,
  246,  250,  251,  252,    2,   18,   24,   40,   47,   58,
   30,   68,   76,   83,   27,   92,  101,  113,   33,   65,
   66,   75,  129,  261,  176,  232,  235,  238,  241,  243,
  245,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   10,    0,    0,    0,   10,   10,   10,
   10,   12,    0,    0,    0,   12,    0,    0,   10,   10,
   10,   10,   10,   10,   10,   10,   12,    0,    0,    0,
    0,    0,    0,   10,   10,    0,   10,    0,    0,   10,
    0,    0,    0,   10,    0,    0,   10,    0,    0,   10,
    0,    0,   10,    0,    0,    0,    0,    0,    0,    0,
   12,
};
static const pcb_bxl_int_t pcb_bxl_check[] = {                   90,
    0,   60,   61,   34,   35,  263,   10,    1,    0,  260,
  263,  290,  136,  260,  267,  261,  262,   11,  260,  277,
   79,   80,   81,  256,   18,  278,   40,  263,  307,  308,
   24,   40,  290,  313,  292,  274,  160,  161,  277,  314,
  298,  277,  166,  258,  274,  303,  304,  277,  258,   43,
  109,  290,  310,  258,  290,   41,  295,  258,  259,  312,
  290,  297,  298,  271,  272,  273,  258,  259,  304,   41,
  300,  301,  302,  274,   68,  134,  277,  258,  269,  138,
   58,  140,   40,   77,   40,  144,   40,  146,  147,  290,
   58,  280,  151,  282,  283,  284,  279,  156,   92,  158,
  159,  225,  226,  311,  305,  256,  165,  285,  274,  275,
  276,  277,  315,  172,  258,  293,  294,  258,  296,   44,
   44,  299,   44,  154,   40,  303,   40,  270,  306,   40,
   40,  309,  191,  224,  264,  265,  266,   40,  169,  170,
  286,  287,  288,  289,  290,  291,  292,   40,   40,  281,
   41,   41,   41,  260,  268,   40,  258,  260,   41,  260,
  260,  258,  257,   44,  260,   41,   41,  257,  260,  257,
  257,  260,  231,   41,  233,  257,   41,  236,  237,  260,
  239,  257,  257,  242,  178,  244,  260,  246,   41,  260,
  260,   41,   44,   44,  260,   44,   44,   44,   44,   44,
   44,  258,  257,   41,    0,   10,   10,  279,  281,   10,
  268,   10,   10,   10,   20,   10,   10,   10,   10,   49,
   56,   63,  106,  252,  123,  186,  192,  197,  204,  211,
  220,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  263,   -1,   -1,   -1,  267,  268,  269,
  270,  263,   -1,   -1,   -1,  267,   -1,   -1,  278,  279,
  280,  281,  282,  283,  284,  285,  278,   -1,   -1,   -1,
   -1,   -1,   -1,  293,  294,   -1,  296,   -1,   -1,  299,
   -1,   -1,   -1,  303,   -1,   -1,  306,   -1,   -1,  309,
   -1,   -1,  312,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  312,
};
#define pcb_bxl_FINAL 2
#define pcb_bxl_MAXTOKEN 315
#define pcb_bxl_UNDFTOKEN 364
#define pcb_bxl_TRANSLATE(a) ((a) > pcb_bxl_MAXTOKEN ? pcb_bxl_UNDFTOKEN : (a))
#if pcb_bxl_DEBUG
static const char *const pcb_bxl_name[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,"'\\n'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,"'('","')'",0,0,"','",0,0,0,0,0,0,0,0,0,0,0,0,0,"':'",0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"T_ID",
"T_INTEGER","T_REAL_ONLY","T_QSTR","T_TRUE","T_FALSE","T_TEXTSTYLE",
"T_FONTWIDTH","T_FONTCHARWIDTH","T_FONTHEIGHT","T_PADSTACK","T_ENDPADSTACK",
"T_SHAPES","T_PADSHAPE","T_HOLEDIAM","T_SURFACE","T_PLATED","T_WIDTH",
"T_HEIGHT","T_PADTYPE","T_LAYER","T_PATTERN","T_ENDPATTERN","T_DATA",
"T_ENDDATA","T_ORIGINPOINT","T_PICKPOINT","T_GLUEPOINT","T_PAD","T_NUMBER",
"T_PINNAME","T_PADSTYLE","T_ORIGINALPADSTYLE","T_ORIGIN","T_ORIGINALPINNUMBER",
"T_ROTATE","T_POLY","T_LINE","T_ENDPOINT","T_ATTRIBUTE","T_ATTR","T_JUSTIFY",
"T_ARC","T_RADIUS","T_STARTANGLE","T_SWEEPANGLE","T_TEXT","T_ISVISIBLE",
"T_PROPERTY","T_WIZARD","T_VARNAME","T_VARDATA","T_TEMPLATEDATA","T_ISFLIPPED",
"T_NOPASTE","T_SYMBOL","T_ENDSYMBOL","T_COMPONENT","T_ENDCOMPONENT",0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,"illegal-symbol",
};
static const char *const pcb_bxl_rule[] = {
"$accept : full_file",
"full_file : maybe_nl file",
"file :",
"file : statement nl file",
"statement : text_style",
"statement : pad_stack",
"statement : pattern",
"statement : boring_section",
"boolean : T_TRUE",
"boolean : T_FALSE",
"nl : '\\n'",
"nl : '\\n' nl",
"maybe_nl :",
"maybe_nl : nl",
"real : T_INTEGER",
"real : T_REAL_ONLY",
"coord : real",
"text_style : T_TEXTSTYLE T_QSTR text_style_attrs",
"text_style_attrs :",
"text_style_attrs : '(' text_style_attr ')' text_style_attrs",
"text_style_attr : T_FONTWIDTH T_INTEGER",
"text_style_attr : T_FONTCHARWIDTH T_INTEGER",
"text_style_attr : T_FONTHEIGHT T_INTEGER",
"pad_stack : T_PADSTACK T_QSTR pstk_attrs nl T_SHAPES ':' T_INTEGER nl pad_shapes T_ENDPADSTACK",
"pstk_attrs :",
"pstk_attrs : '(' pstk_attr ')' pstk_attrs",
"pstk_attr : T_HOLEDIAM T_INTEGER",
"pstk_attr : T_SURFACE boolean",
"pstk_attr : T_PLATED boolean",
"pstk_attr : T_NOPASTE boolean",
"pad_shapes :",
"pad_shapes : pad_shape pad_shapes",
"pad_shape : T_PADSHAPE T_QSTR padshape_attrs nl",
"padshape_attrs :",
"padshape_attrs : '(' padshape_attr ')' padshape_attrs",
"padshape_attr : T_WIDTH real",
"padshape_attr : T_HEIGHT real",
"padshape_attr : T_PADTYPE T_INTEGER",
"padshape_attr : T_LAYER T_ID",
"pattern : T_PATTERN T_QSTR nl pattern_chldrn T_ENDPATTERN",
"pattern_chldrn :",
"pattern_chldrn : pattern_chld nl pattern_chldrn",
"pattern_chld : data",
"pattern_chld : T_ORIGINPOINT '(' coord ',' coord ')'",
"pattern_chld : T_PICKPOINT '(' coord ',' coord ')'",
"pattern_chld : T_GLUEPOINT '(' coord ',' coord ')'",
"data : T_DATA ':' T_INTEGER nl data_chldrn T_ENDDATA",
"data_chldrn :",
"data_chldrn : data_chld nl data_chldrn",
"data_chld : pad",
"data_chld : poly",
"data_chld : line",
"data_chld : attribute",
"data_chld : templatedata",
"data_chld : arc",
"data_chld : text",
"data_chld : wizard",
"pad : T_PAD pad_attrs",
"pad_attrs :",
"pad_attrs : '(' pad_attr ')' pad_attrs",
"pad_attr : T_NUMBER T_INTEGER",
"pad_attr : T_PINNAME T_QSTR",
"pad_attr : T_PADSTYLE T_QSTR",
"pad_attr : T_ORIGINALPADSTYLE T_QSTR",
"pad_attr : T_ORIGIN coord ',' coord",
"pad_attr : T_ORIGINALPINNUMBER T_INTEGER",
"pad_attr : T_ROTATE real",
"poly : T_POLY poly_attrs",
"poly_attrs :",
"poly_attrs : '(' poly_attr ')' poly_attrs",
"poly_attr : T_LAYER T_ID",
"poly_attr : T_ORIGIN coord ',' coord",
"poly_attr : T_WIDTH coord",
"poly_attr : T_PROPERTY T_QSTR",
"poly_attr : coord ',' coord",
"line : T_LINE line_attrs",
"line_attrs :",
"line_attrs : '(' line_attr ')' line_attrs",
"line_attr : T_LAYER T_ID",
"line_attr : T_ORIGIN coord ',' coord",
"line_attr : T_ENDPOINT coord ',' coord",
"line_attr : T_WIDTH coord",
"attribute : T_ATTRIBUTE attribute_attrs",
"attribute_attrs :",
"attribute_attrs : '(' attribute_attr ')' attribute_attrs",
"attribute_attr : T_LAYER T_ID",
"attribute_attr : T_ORIGIN coord ',' coord",
"attribute_attr : T_ATTR T_QSTR T_QSTR",
"attribute_attr : T_JUSTIFY T_ID",
"attribute_attr : T_TEXTSTYLE T_QSTR",
"attribute_attr : T_ISVISIBLE boolean",
"arc : T_ARC arc_attrs",
"arc_attrs :",
"arc_attrs : '(' arc_attr ')' arc_attrs",
"arc_attr : T_LAYER T_ID",
"arc_attr : T_ORIGIN coord ',' coord",
"arc_attr : T_WIDTH coord",
"arc_attr : T_RADIUS coord",
"arc_attr : T_STARTANGLE real",
"arc_attr : T_SWEEPANGLE real",
"text : T_TEXT text_attrs",
"text_attrs :",
"text_attrs : '(' text_attr ')' text_attrs",
"text_attr : T_LAYER T_ID",
"text_attr : T_ORIGIN coord ',' coord",
"text_attr : T_TEXT T_QSTR",
"text_attr : T_ISVISIBLE boolean",
"text_attr : T_JUSTIFY T_ID",
"text_attr : T_TEXTSTYLE T_QSTR",
"text_attr : T_ISFLIPPED boolean",
"text_attr : T_ROTATE real",
"wizard : T_WIZARD wizard_attrs",
"templatedata : T_TEMPLATEDATA wizard_attrs",
"wizard_attrs :",
"wizard_attrs : '(' wizard_attr ')' wizard_attrs",
"wizard_attr : T_ORIGIN coord ',' coord",
"wizard_attr : T_VARNAME T_QSTR",
"wizard_attr : T_VARDATA T_QSTR",
"boring_section : T_SYMBOL error T_ENDSYMBOL T_COMPONENT error T_ENDCOMPONENT",

};
#endif


#if pcb_bxl_DEBUG
#include <stdio.h> /* needed for printf */
#endif

#include <stdlib.h> /* needed for malloc, etc */
#include <string.h> /* needed for memset */

/* allocate initial stack or double stack size, up to yyctx->stack_max_depth */
static int pcb_bxl_growstack(pcb_bxl_yyctx_t *yyctx, pcb_bxl_STACKDATA *data)
{
	int i;
	unsigned newsize;
	pcb_bxl_int_t *newss;
	pcb_bxl_STYPE *newvs;

	if ((newsize = data->stacksize) == 0)
		newsize = pcb_bxl_INITSTACKSIZE;
	else if (newsize >= yyctx->stack_max_depth)
		return pcb_bxl_ENOMEM;
	else if ((newsize *= 2) > yyctx->stack_max_depth)
		newsize = yyctx->stack_max_depth;

	i = (int)(data->s_mark - data->s_base);
	newss = (pcb_bxl_int_t *) realloc(data->s_base, newsize * sizeof(*newss));
	if (newss == 0)
		return pcb_bxl_ENOMEM;

	data->s_base = newss;
	data->s_mark = newss + i;

	newvs = (pcb_bxl_STYPE *) realloc(data->l_base, newsize * sizeof(*newvs));
	if (newvs == 0)
		return pcb_bxl_ENOMEM;

	data->l_base = newvs;
	data->l_mark = newvs + i;

	data->stacksize = newsize;
	data->s_last = data->s_base + newsize - 1;
	return 0;
}

static void pcb_bxl_freestack(pcb_bxl_STACKDATA *data)
{
	free(data->s_base);
	free(data->l_base);
	memset(data, 0, sizeof(*data));
}

#define pcb_bxl_ABORT  goto yyabort
#define pcb_bxl_REJECT goto yyabort
#define pcb_bxl_ACCEPT goto yyaccept
#define pcb_bxl_ERROR  goto yyerrlab

int pcb_bxl_parse_init(pcb_bxl_yyctx_t *yyctx)
{
#if pcb_bxl_DEBUG
	const char *yys;

	if ((yys = getenv("pcb_bxl_DEBUG")) != 0) {
		yyctx->yyn = *yys;
		if (yyctx->yyn >= '0' && yyctx->yyn <= '9')
			yyctx->debug = yyctx->yyn - '0';
	}
#endif

	memset(&yyctx->val, 0, sizeof(yyctx->val));
	memset(&yyctx->lval, 0, sizeof(yyctx->lval));

	yyctx->yym = 0;
	yyctx->yyn = 0;
	yyctx->nerrs = 0;
	yyctx->errflag = 0;
	yyctx->chr = pcb_bxl_EMPTY;
	yyctx->state = 0;

	memset(&yyctx->stack, 0, sizeof(yyctx->stack));

	yyctx->stack_max_depth = pcb_bxl_INITSTACKSIZE > 10000 ? pcb_bxl_INITSTACKSIZE : 10000;
	if (yyctx->stack.s_base == NULL && pcb_bxl_growstack(yyctx, &yyctx->stack) == pcb_bxl_ENOMEM)
		return -1;
	yyctx->stack.s_mark = yyctx->stack.s_base;
	yyctx->stack.l_mark = yyctx->stack.l_base;
	yyctx->state = 0;
	*yyctx->stack.s_mark = 0;
	yyctx->jump = 0;
	return 0;
}


#define pcb_bxl_GETCHAR(labidx) \
do { \
	if (used)               { yyctx->jump = labidx; return pcb_bxl_RES_NEXT; } \
	getchar_ ## labidx:;    yyctx->chr = tok; yyctx->lval = *lval; used = 1; \
} while(0)

pcb_bxl_res_t pcb_bxl_parse(pcb_bxl_yyctx_t *yyctx, pcb_bxl_ctx_t *ctx, int tok, pcb_bxl_STYPE *lval)
{
	int used = 0;
#if pcb_bxl_DEBUG
	const char *yys;
#endif

yyloop:;
	if (yyctx->jump == 1) { yyctx->jump = 0; goto getchar_1; }
	if (yyctx->jump == 2) { yyctx->jump = 0; goto getchar_2; }

	if ((yyctx->yyn = pcb_bxl_defred[yyctx->state]) != 0)
		goto yyreduce;
	if (yyctx->chr < 0) {
		pcb_bxl_GETCHAR(1);
		if (yyctx->chr < 0)
			yyctx->chr = pcb_bxl_EOF;
#if pcb_bxl_DEBUG
		if (yyctx->debug) {
			if ((yys = pcb_bxl_name[pcb_bxl_TRANSLATE(yyctx->chr)]) == NULL)
				yys = pcb_bxl_name[pcb_bxl_UNDFTOKEN];
			printf("yyctx->debug: state %d, reading %d (%s)\n", yyctx->state, yyctx->chr, yys);
		}
#endif
	}
	if (((yyctx->yyn = pcb_bxl_sindex[yyctx->state]) != 0) && (yyctx->yyn += yyctx->chr) >= 0 && yyctx->yyn <= pcb_bxl_TABLESIZE && pcb_bxl_check[yyctx->yyn] == (pcb_bxl_int_t) yyctx->chr) {
#if pcb_bxl_DEBUG
		if (yyctx->debug)
			printf("yyctx->debug: state %d, shifting to state %d\n", yyctx->state, pcb_bxl_table[yyctx->yyn]);
#endif
		if (yyctx->stack.s_mark >= yyctx->stack.s_last && pcb_bxl_growstack(yyctx, &yyctx->stack) == pcb_bxl_ENOMEM)
			goto yyoverflow;
		yyctx->state = pcb_bxl_table[yyctx->yyn];
		*++yyctx->stack.s_mark = pcb_bxl_table[yyctx->yyn];
		*++yyctx->stack.l_mark = yyctx->lval;
		yyctx->chr = pcb_bxl_EMPTY;
		if (yyctx->errflag > 0)
			--yyctx->errflag;
		goto yyloop;
	}
	if (((yyctx->yyn = pcb_bxl_rindex[yyctx->state]) != 0) && (yyctx->yyn += yyctx->chr) >= 0 && yyctx->yyn <= pcb_bxl_TABLESIZE && pcb_bxl_check[yyctx->yyn] == (pcb_bxl_int_t) yyctx->chr) {
		yyctx->yyn = pcb_bxl_table[yyctx->yyn];
		goto yyreduce;
	}
	if (yyctx->errflag != 0)
		goto yyinrecovery;

	pcb_bxl_error(ctx, yyctx->lval, "syntax error");

	goto yyerrlab;	/* redundant goto avoids 'unused label' warning */
yyerrlab:
	++yyctx->nerrs;

yyinrecovery:
	if (yyctx->errflag < 3) {
		yyctx->errflag = 3;
		for(;;) {
			if (((yyctx->yyn = pcb_bxl_sindex[*yyctx->stack.s_mark]) != 0) && (yyctx->yyn += pcb_bxl_ERRCODE) >= 0 && yyctx->yyn <= pcb_bxl_TABLESIZE && pcb_bxl_check[yyctx->yyn] == (pcb_bxl_int_t) pcb_bxl_ERRCODE) {
#if pcb_bxl_DEBUG
				if (yyctx->debug)
					printf("yyctx->debug: state %d, error recovery shifting to state %d\n", *yyctx->stack.s_mark, pcb_bxl_table[yyctx->yyn]);
#endif
				if (yyctx->stack.s_mark >= yyctx->stack.s_last && pcb_bxl_growstack(yyctx, &yyctx->stack) == pcb_bxl_ENOMEM)
					goto yyoverflow;
				yyctx->state = pcb_bxl_table[yyctx->yyn];
				*++yyctx->stack.s_mark = pcb_bxl_table[yyctx->yyn];
				*++yyctx->stack.l_mark = yyctx->lval;
				goto yyloop;
			}
			else {
#if pcb_bxl_DEBUG
				if (yyctx->debug)
					printf("yyctx->debug: error recovery discarding state %d\n", *yyctx->stack.s_mark);
#endif
				if (yyctx->stack.s_mark <= yyctx->stack.s_base)
					goto yyabort;
				--yyctx->stack.s_mark;
				--yyctx->stack.l_mark;
			}
		}
	}
	else {
		if (yyctx->chr == pcb_bxl_EOF)
			goto yyabort;
#if pcb_bxl_DEBUG
		if (yyctx->debug) {
			if ((yys = pcb_bxl_name[pcb_bxl_TRANSLATE(yyctx->chr)]) == NULL)
				yys = pcb_bxl_name[pcb_bxl_UNDFTOKEN];
			printf("yyctx->debug: state %d, error recovery discards token %d (%s)\n", yyctx->state, yyctx->chr, yys);
		}
#endif
		yyctx->chr = pcb_bxl_EMPTY;
		goto yyloop;
	}

yyreduce:
#if pcb_bxl_DEBUG
	if (yyctx->debug)
		printf("yyctx->debug: state %d, reducing by rule %d (%s)\n", yyctx->state, yyctx->yyn, pcb_bxl_rule[yyctx->yyn]);
#endif
	yyctx->yym = pcb_bxl_len[yyctx->yyn];
	if (yyctx->yym > 0)
		yyctx->val = yyctx->stack.l_mark[1 - yyctx->yym];
	else
		memset(&yyctx->val, 0, sizeof yyctx->val);

	switch (yyctx->yyn) {
	}
	yyctx->stack.s_mark -= yyctx->yym;
	yyctx->state = *yyctx->stack.s_mark;
	yyctx->stack.l_mark -= yyctx->yym;
	yyctx->yym = pcb_bxl_lhs[yyctx->yyn];
	if (yyctx->state == 0 && yyctx->yym == 0) {
#if pcb_bxl_DEBUG
		if (yyctx->debug)
			printf("yyctx->debug: after reduction, shifting from state 0 to state %d\n", pcb_bxl_FINAL);
#endif
		yyctx->state = pcb_bxl_FINAL;
		*++yyctx->stack.s_mark = pcb_bxl_FINAL;
		*++yyctx->stack.l_mark = yyctx->val;
		if (yyctx->chr < 0) {
			pcb_bxl_GETCHAR(2);
			if (yyctx->chr < 0)
				yyctx->chr = pcb_bxl_EOF;
#if pcb_bxl_DEBUG
			if (yyctx->debug) {
				if ((yys = pcb_bxl_name[pcb_bxl_TRANSLATE(yyctx->chr)]) == NULL)
					yys = pcb_bxl_name[pcb_bxl_UNDFTOKEN];
				printf("yyctx->debug: state %d, reading %d (%s)\n", pcb_bxl_FINAL, yyctx->chr, yys);
			}
#endif
		}
		if (yyctx->chr == pcb_bxl_EOF)
			goto yyaccept;
		goto yyloop;
	}
	if (((yyctx->yyn = pcb_bxl_gindex[yyctx->yym]) != 0) && (yyctx->yyn += yyctx->state) >= 0 && yyctx->yyn <= pcb_bxl_TABLESIZE && pcb_bxl_check[yyctx->yyn] == (pcb_bxl_int_t) yyctx->state)
		yyctx->state = pcb_bxl_table[yyctx->yyn];
	else
		yyctx->state = pcb_bxl_dgoto[yyctx->yym];
#if pcb_bxl_DEBUG
	if (yyctx->debug)
		printf("yyctx->debug: after reduction, shifting from state %d to state %d\n", *yyctx->stack.s_mark, yyctx->state);
#endif
	if (yyctx->stack.s_mark >= yyctx->stack.s_last && pcb_bxl_growstack(yyctx, &yyctx->stack) == pcb_bxl_ENOMEM)
		goto yyoverflow;
	*++yyctx->stack.s_mark = (pcb_bxl_int_t) yyctx->state;
	*++yyctx->stack.l_mark = yyctx->val;
	goto yyloop;

yyoverflow:
	pcb_bxl_error(ctx, yyctx->lval, "yacc stack overflow");

yyabort:
	pcb_bxl_freestack(&yyctx->stack);
	return pcb_bxl_RES_ABORT;

yyaccept:
	pcb_bxl_freestack(&yyctx->stack);
	return pcb_bxl_RES_DONE;
}
