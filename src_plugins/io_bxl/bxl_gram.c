
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
    0,    5,    5,    6,    6,    6,    6,    3,    3,    7,
    7,    4,    4,    1,    1,    2,   12,   12,   12,   13,
   14,   15,   17,    8,   16,   16,   18,   18,   18,    9,
   19,   19,   21,   21,   21,   21,   20,   20,   22,   23,
   23,   24,   24,   24,   24,   26,   10,   25,   25,   27,
   27,   27,   27,   28,   29,   29,   30,   30,   30,   30,
   30,   30,   30,   30,   31,   39,   39,   40,   40,   40,
   40,   40,   40,   40,   42,   32,   41,   41,   43,   43,
   43,   43,   43,   45,   33,   44,   44,   46,   46,   46,
   46,   34,   47,   47,   48,   48,   48,   48,   50,   36,
   49,   49,   51,   51,   51,   51,   51,   51,   53,   37,
   52,   52,   54,   54,   54,   54,   54,   54,   38,   35,
   55,   55,   56,   56,   56,   11,
};
static const pcb_bxl_int_t pcb_bxl_len[] = {                      2,
    2,    0,    3,    1,    1,    1,    1,    1,    1,    1,
    2,    0,    1,    1,    1,    1,    2,    2,    2,    4,
    2,    2,    0,    4,    0,    4,    2,    2,    2,   10,
    0,    4,    2,    2,    2,    2,    0,    2,    4,    0,
    4,    2,    2,    1,    1,    0,    6,    0,    3,    1,
    6,    6,    6,    6,    0,    3,    1,    1,    1,    1,
    1,    1,    1,    1,    2,    0,    4,    2,    2,    2,
    2,    2,    2,    1,    0,    3,    0,    4,    2,    3,
    1,    1,    1,    0,    3,    0,    4,    4,    1,    1,
    1,    2,    0,    4,    3,    1,    1,    1,    0,    3,
    0,    4,    2,    2,    2,    1,    1,    1,    0,    3,
    0,    4,    2,    2,    2,    1,    1,    1,    2,    2,
    0,    4,    2,    2,    1,    6,
};
static const pcb_bxl_int_t pcb_bxl_defred[] = {                   0,
    0,    0,    0,   13,   11,    0,    0,    0,    0,    1,
    0,    4,    5,    6,    7,   23,    0,    0,    0,    0,
    0,    0,    0,   46,    0,    3,    0,   24,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   33,    8,    9,   34,   35,   36,    0,    0,    0,    0,
    0,    0,    0,    0,   50,    0,   14,   15,   27,   28,
   29,    0,   32,    0,    0,    0,    0,    0,   47,    0,
  126,   26,    0,    0,   16,    0,    0,    0,   49,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   75,   84,
    0,   99,  109,    0,    0,    0,    0,   57,   58,   59,
   60,   61,   62,   63,   64,    0,    0,    0,    0,   30,
   38,    0,   65,    0,    0,    0,   92,    0,    0,    0,
  119,  120,   54,    0,   51,   52,   53,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   74,    0,    0,   76,
    0,   85,    0,    0,    0,    0,    0,   96,   97,   98,
    0,    0,  100,    0,  110,    0,    0,  125,    0,   56,
    0,    0,    0,   44,   45,    0,   39,   68,   69,   70,
   71,    0,   72,   73,    0,    0,    0,   81,   82,   83,
    0,    0,   89,   90,   91,    0,   18,   21,    0,   17,
   19,    0,    0,    0,    0,  106,  107,  108,    0,    0,
    0,    0,  116,  117,  118,    0,  123,  124,    0,   22,
   42,   43,    0,    0,   67,   79,    0,    0,    0,    0,
   95,   94,  103,  104,  105,    0,  115,  113,  114,    0,
  122,   41,   20,   80,   78,    0,   87,  102,  112,   88,
};
static const pcb_bxl_int_t pcb_bxl_dgoto[] = {                    2,
   75,   76,   44,    3,   10,   11,    4,   12,   13,   14,
   15,  148,  137,  150,  165,   28,   21,   40,   23,   86,
   33,   87,  129,  166,   53,   35,   54,   55,   96,   97,
   98,   99,  100,  101,  102,  103,  104,  105,  113,  138,
  140,  114,  181,  142,  115,  186,  117,  151,  153,  118,
  199,  155,  119,  206,  121,  159,
};
static const pcb_bxl_int_t pcb_bxl_sindex[] = {                  13,
   13,    0, -259,    0,    0, -250, -221, -218, -197,    0,
   13,    0,    0,    0,    0,    0,   27,   13, -244, -259,
   34, -245,   13,    0, -239,    0, -169,    0, -180, -229,
 -229, -229,   59, -168, -196, -154, -203, -203, -203,   63,
    0,    0,    0,    0,    0,    0,   27,   47,   48,   67,
   72,   73, -163,   13,    0, -198,    0,    0,    0,    0,
    0,   34,    0, -139, -137, -203, -203, -203,    0, -196,
    0,    0,   13,   13,    0,   78,   81,   83,    0, -142,
 -170, -203, -203, -203, -130, -136, -142,   94,    0,    0,
   98,    0,    0,  107,  107, -133,   13,    0,    0,    0,
    0,    0,    0,    0,    0,  108,  109,  110,  113,    0,
    0, -146,    0,  114,  116, -233,    0,  117,  118, -272,
    0,    0,    0, -170,    0,    0,    0, -184,   13,  -99,
 -100,  -97,  -96, -203,  -93, -203,    0,  120, -243,    0,
 -201,    0,  -94,  -90,  -92,  -86, -229,    0,    0,    0,
  121, -192,    0, -252,    0,  -88,  -84,    0,  136,    0,
 -203, -203,  -80,    0,    0,  138,    0,    0,    0,    0,
    0,  137,    0,    0,   94,  -78,  139,    0,    0,    0,
  143, -203,    0,    0,    0,  144,    0,    0,  -74,    0,
    0,   98, -203, -203, -203,    0,    0,    0,  146, -203,
  -72, -229,    0,    0,    0,  148,    0,    0,  107,    0,
    0,    0,  113, -203,    0,    0, -203,  114,  147,  116,
    0,    0,    0,    0,    0,  117,    0,    0,    0,  118,
    0,    0,    0,    0,    0, -203,    0,    0,    0,    0,
};
static const pcb_bxl_int_t pcb_bxl_rindex[] = {                   9,
    1,    0,  180,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  182,    0,    0,  180,
  183,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  -89,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  182,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  183,    0,    0,    0,    0,    0,    0,    0,  -89,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  -73,
  -87,    0,    0,    0,    0,    0,  -73,  186,    0,    0,
  187,    0,    0,  188,  188,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  189,    0,
    0,    0,    0,  190,  191,    0,    0,  192,  193,    0,
    0,    0,    0,  -87,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  186,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  187,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  188,    0,
    0,    0,  189,    0,    0,    0,    0,  190,    0,  191,
    0,    0,    0,    0,    0,  192,    0,    0,    0,  193,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
static const pcb_bxl_int_t pcb_bxl_gindex[] = {                   0,
  -25,  -62,  -29,    0,  184,    0,    6,    0,    0,    0,
    0,   51,  -71,  -91,  -98,  145,    0,    0,  159,  122,
    0,    0,   -5,    0,  140,    0,    0,    0,   87,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   37,    0,
   -4,    0,    0,   -7,    0,    0,   23,    0,  -10,    0,
    0,  -13,    0,    0,  -95,    0,
};
#define pcb_bxl_TABLESIZE 321
static const pcb_bxl_int_t pcb_bxl_table[] = {                  122,
   10,   45,   46,    6,   77,   78,    5,    7,   12,   16,
  143,   59,   60,   61,   57,   58,   20,  134,    8,  106,
  107,  108,    1,   24,  144,   29,   30,   31,   34,  143,
  161,   42,   43,  144,  156,  157,  164,  134,   17,  200,
  180,   18,  185,  144,  149,  146,  134,  179,  158,  184,
  201,  147,    9,  198,   57,   58,  134,  202,   19,   70,
  197,  176,  205,  145,  146,   32,   22,  178,   25,  183,
  147,  172,  161,   27,   36,  144,  177,   41,   80,   81,
  196,  161,  204,   49,  144,   50,   51,   52,  134,  161,
  162,  163,  144,  182,   37,   38,   39,  134,  210,   47,
   48,   56,  124,   62,   64,   65,   66,  193,  194,  195,
  174,   67,   68,  231,   88,   69,   71,  191,   73,  219,
   74,   82,   89,   90,   83,   91,   84,   85,   92,  109,
  223,  110,   93,  112,  167,   94,  211,  116,   95,  130,
  131,  132,  133,  134,  135,  136,  120,  123,  125,  126,
  127,  233,  128,  139,  234,  141,  152,  154,  168,  169,
  175,  192,  170,  171,  173,  187,  188,  189,  224,  225,
  190,  207,  229,  240,  227,  208,  209,  212,  213,    2,
  214,  216,  217,  218,  220,  221,  226,  228,  230,   48,
  236,   31,   25,   55,   37,   66,   93,  121,   40,   77,
   86,  101,  111,   26,  203,   63,   72,  232,  111,   79,
  160,  215,  237,  235,  222,  238,  239,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
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
static const pcb_bxl_int_t pcb_bxl_check[] = {                   95,
    0,   31,   32,  263,   67,   68,    1,  267,    0,  260,
  263,   37,   38,   39,  258,  259,   11,  290,  278,   82,
   83,   84,   10,   18,  277,  271,  272,  273,   23,  263,
  274,  261,  262,  277,  307,  308,  128,  290,  260,  292,
  139,  260,  141,  277,  116,  298,  290,  139,  120,  141,
  303,  304,  312,  152,  258,  259,  290,  310,  256,   54,
  152,  305,  154,  297,  298,  311,   40,  139,  313,  141,
  304,  134,  274,   40,  314,  277,  139,  258,   73,   74,
  152,  274,  154,  280,  277,  282,  283,  284,  290,  274,
  275,  276,  277,  295,  264,  265,  266,  290,  161,   41,
  269,  256,   97,   41,   58,   58,   40,  300,  301,  302,
  136,   40,   40,  209,  285,  279,  315,  147,  258,  182,
  258,   44,  293,  294,   44,  296,   44,  270,  299,  260,
  193,  268,  303,   40,  129,  306,  162,   40,  309,  286,
  287,  288,  289,  290,  291,  292,   40,  281,   41,   41,
   41,  214,   40,   40,  217,   40,   40,   40,  258,  260,
   41,   41,  260,  260,  258,  260,  257,  260,  194,  195,
  257,  260,  202,  236,  200,  260,   41,  258,   41,    0,
   44,  260,   44,   41,   41,  260,   41,  260,   41,  279,
   44,   10,   10,  281,  268,   10,   10,   10,   10,   10,
   10,   10,   10,   20,  154,   47,   62,  213,   87,   70,
  124,  175,  220,  218,  192,  226,  230,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
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
#define pcb_bxl_UNDFTOKEN 374
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
0,0,0,0,0,0,0,0,0,0,0,0,"illegal-symbol",
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
"common_attr_text : T_JUSTIFY T_ID",
"common_attr_text : T_TEXTSTYLE T_QSTR",
"common_attr_text : T_ISVISIBLE boolean",
"common_origin : T_ORIGIN coord ',' coord",
"common_layer : T_LAYER T_ID",
"common_width : T_WIDTH coord",
"$$1 :",
"text_style : T_TEXTSTYLE T_QSTR $$1 text_style_attrs",
"text_style_attrs :",
"text_style_attrs : '(' text_style_attr ')' text_style_attrs",
"text_style_attr : T_FONTWIDTH real",
"text_style_attr : T_FONTCHARWIDTH real",
"text_style_attr : T_FONTHEIGHT real",
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
"padshape_attr : T_HEIGHT real",
"padshape_attr : T_PADTYPE T_INTEGER",
"padshape_attr : common_layer",
"padshape_attr : common_width",
"$$2 :",
"pattern : T_PATTERN T_QSTR nl $$2 pattern_chldrn T_ENDPATTERN",
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
"pad_attr : T_ORIGINALPINNUMBER T_INTEGER",
"pad_attr : T_ROTATE real",
"pad_attr : common_origin",
"$$3 :",
"poly : T_POLY $$3 poly_attrs",
"poly_attrs :",
"poly_attrs : '(' poly_attr ')' poly_attrs",
"poly_attr : T_PROPERTY T_QSTR",
"poly_attr : coord ',' coord",
"poly_attr : common_origin",
"poly_attr : common_layer",
"poly_attr : common_width",
"$$4 :",
"line : T_LINE $$4 line_attrs",
"line_attrs :",
"line_attrs : '(' line_attr ')' line_attrs",
"line_attr : T_ENDPOINT coord ',' coord",
"line_attr : common_origin",
"line_attr : common_layer",
"line_attr : common_width",
"attribute : T_ATTRIBUTE attribute_attrs",
"attribute_attrs :",
"attribute_attrs : '(' attribute_attr ')' attribute_attrs",
"attribute_attr : T_ATTR T_QSTR T_QSTR",
"attribute_attr : common_attr_text",
"attribute_attr : common_origin",
"attribute_attr : common_layer",
"$$5 :",
"arc : T_ARC $$5 arc_attrs",
"arc_attrs :",
"arc_attrs : '(' arc_attr ')' arc_attrs",
"arc_attr : T_RADIUS coord",
"arc_attr : T_STARTANGLE real",
"arc_attr : T_SWEEPANGLE real",
"arc_attr : common_origin",
"arc_attr : common_layer",
"arc_attr : common_width",
"$$6 :",
"text : T_TEXT $$6 text_attrs",
"text_attrs :",
"text_attrs : '(' text_attr ')' text_attrs",
"text_attr : T_TEXT T_QSTR",
"text_attr : T_ISFLIPPED boolean",
"text_attr : T_ROTATE real",
"text_attr : common_attr_text",
"text_attr : common_origin",
"text_attr : common_layer",
"wizard : T_WIZARD wizard_attrs",
"templatedata : T_TEMPLATEDATA wizard_attrs",
"wizard_attrs :",
"wizard_attrs : '(' wizard_attr ')' wizard_attrs",
"wizard_attr : T_VARNAME T_QSTR",
"wizard_attr : T_VARDATA T_QSTR",
"wizard_attr : common_origin",
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
case 8:
#line 101 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.i = 1; }
break;
case 9:
#line 102 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.i = 0; }
break;
case 14:
#line 116 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.d = yyctx->stack.l_mark[0].un.i; }
break;
case 15:
#line 117 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.d = yyctx->stack.l_mark[0].un.d; }
break;
case 16:
#line 121 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.c = PCB_MIL_TO_COORD(yyctx->stack.l_mark[0].un.d); }
break;
case 17:
#line 125 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_justify(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 18:
#line 126 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_text_style(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 19:
#line 127 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.invis = yyctx->stack.l_mark[0].un.i; }
break;
case 20:
#line 131 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.origin_x = yyctx->stack.l_mark[-2].un.c; ctx->state.origin_y = yyctx->stack.l_mark[0].un.c; }
break;
case 21:
#line 135 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_layer(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 22:
#line 139 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.width = yyctx->stack.l_mark[0].un.c; }
break;
case 23:
#line 145 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_text_style_begin(ctx, yyctx->stack.l_mark[0].un.s); /* $2 is taken over */ }
break;
case 24:
#line 146 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_text_style_end(ctx); }
break;
case 27:
#line 155 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.text_style->width = yyctx->stack.l_mark[0].un.d; }
break;
case 28:
#line 156 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.text_style->char_width = yyctx->stack.l_mark[0].un.d; }
break;
case 29:
#line 157 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.text_style->height = yyctx->stack.l_mark[0].un.d; }
break;
case 34:
#line 177 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.surface = yyctx->stack.l_mark[0].un.i; }
break;
case 35:
#line 178 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.plated = yyctx->stack.l_mark[0].un.i; }
break;
case 36:
#line 179 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.nopaste = yyctx->stack.l_mark[0].un.i; }
break;
case 46:
#line 205 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_begin(ctx, yyctx->stack.l_mark[-1].un.s); free(yyctx->stack.l_mark[-1].un.s); }
break;
case 47:
#line 207 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_end(ctx); }
break;
case 75:
#line 267 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_begin(ctx); }
break;
case 76:
#line 268 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_end(ctx); }
break;
case 79:
#line 278 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_property(ctx, (pcb_any_obj_t *)ctx->state.poly, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 80:
#line 279 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_add_vertex(ctx, yyctx->stack.l_mark[-2].un.c, yyctx->stack.l_mark[0].un.c); }
break;
case 84:
#line 287 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 85:
#line 288 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_line(ctx); pcb_bxl_reset(ctx); }
break;
case 88:
#line 297 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.endp_x = yyctx->stack.l_mark[-2].un.c; ctx->state.endp_y = yyctx->stack.l_mark[0].un.c; }
break;
case 99:
#line 323 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 100:
#line 324 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_arc(ctx); pcb_bxl_reset(ctx); }
break;
case 103:
#line 333 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.radius = yyctx->stack.l_mark[0].un.c;  }
break;
case 104:
#line 334 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.arc_start = yyctx->stack.l_mark[0].un.d; }
break;
case 105:
#line 335 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.arc_delta = yyctx->stack.l_mark[0].un.d; }
break;
case 109:
#line 343 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 110:
#line 344 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_text(ctx); pcb_bxl_reset(ctx); }
break;
case 113:
#line 353 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_text_str(ctx, yyctx->stack.l_mark[0].un.s); /* $2 is taken over */ }
break;
case 114:
#line 354 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.flipped = yyctx->stack.l_mark[0].un.i; }
break;
case 115:
#line 355 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.rot = yyctx->stack.l_mark[0].un.d; }
break;
#line 739 "../../src_plugins/io_bxl/bxl_gram.c"
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
