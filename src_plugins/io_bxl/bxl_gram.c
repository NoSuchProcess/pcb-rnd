
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
   14,   15,   17,    8,   16,   16,   18,   18,   18,   20,
   22,    9,   19,   19,   23,   23,   23,   23,   21,   21,
   26,   24,   25,   25,   27,   27,   27,   27,   29,   10,
   28,   28,   30,   30,   30,   30,   31,   32,   32,   33,
   33,   33,   33,   33,   33,   33,   33,   43,   34,   42,
   42,   44,   44,   44,   44,   44,   44,   44,   46,   35,
   45,   45,   47,   47,   47,   47,   47,   49,   36,   48,
   48,   50,   50,   50,   50,   52,   37,   51,   51,   53,
   53,   53,   53,   55,   39,   54,   54,   56,   56,   56,
   56,   56,   56,   58,   40,   57,   57,   59,   59,   59,
   59,   59,   59,   41,   38,   60,   60,   61,   61,   61,
   11,
};
static const pcb_bxl_int_t pcb_bxl_len[] = {                      2,
    2,    0,    3,    1,    1,    1,    1,    1,    1,    1,
    2,    0,    1,    1,    1,    1,    2,    2,    2,    4,
    2,    2,    0,    4,    0,    4,    2,    2,    2,    0,
    0,   12,    0,    4,    2,    2,    2,    2,    0,    2,
    0,    5,    0,    4,    2,    2,    1,    1,    0,    6,
    0,    3,    1,    6,    6,    6,    6,    0,    3,    1,
    1,    1,    1,    1,    1,    1,    1,    0,    3,    0,
    4,    2,    2,    2,    2,    2,    2,    1,    0,    3,
    0,    4,    2,    3,    1,    1,    1,    0,    3,    0,
    4,    4,    1,    1,    1,    0,    3,    0,    4,    3,
    1,    1,    1,    0,    3,    0,    4,    2,    2,    2,
    1,    1,    1,    0,    3,    0,    4,    2,    2,    2,
    1,    1,    1,    2,    2,    0,    4,    2,    2,    1,
    6,
};
static const pcb_bxl_int_t pcb_bxl_defred[] = {                   0,
    0,    0,    0,   13,   11,    0,    0,    0,    0,    1,
    0,    4,    5,    6,    7,   23,   30,    0,    0,    0,
    0,    0,   49,    0,    3,    0,   24,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   53,    0,   14,
   15,   27,   28,   29,    0,   16,   35,    8,    9,   36,
   37,   38,    0,    0,    0,    0,    0,    0,   50,    0,
  131,   26,   34,    0,    0,    0,    0,    0,   52,    0,
    0,    0,    0,    0,   31,   68,   79,   88,   96,  104,
  114,    0,    0,    0,    0,   60,   61,   62,   63,   64,
   65,   66,   67,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  124,  125,   57,    0,   54,   55,
   56,    0,    0,    0,    0,   69,    0,   80,    0,   89,
    0,   97,    0,  105,    0,  115,    0,    0,    0,  130,
    0,   59,   41,   32,   40,    0,    0,    0,    0,    0,
    0,   78,    0,    0,    0,    0,    0,   85,   86,   87,
    0,    0,   93,   94,   95,    0,    0,    0,    0,    0,
  101,  102,  103,    0,    0,    0,    0,  111,  112,  113,
    0,    0,    0,    0,  121,  122,  123,    0,    0,  128,
  129,    0,    0,   72,   73,   74,   75,   76,   77,    0,
   22,   21,   83,    0,    0,    0,    0,   18,    0,   17,
   19,    0,  108,  109,  110,    0,  120,  118,  119,    0,
    0,  127,    0,    0,   71,   84,   82,    0,   91,  100,
   99,  107,  117,   20,    0,    0,   47,   48,    0,   42,
   92,   45,   46,    0,   44,
};
static const pcb_bxl_int_t pcb_bxl_dgoto[] = {                    2,
   56,   57,   60,    3,   10,   11,    4,   12,   13,   14,
   15,  171,  140,  159,  160,   27,   21,   35,   29,   22,
  123,  107,   40,  124,  224,  193,  239,   46,   30,   47,
   48,   94,   95,   96,   97,   98,   99,  100,  101,  102,
  103,  126,  108,  153,  128,  109,  161,  130,  110,  166,
  132,  111,  174,  134,  112,  181,  136,  113,  188,  115,
  141,
};
static const pcb_bxl_int_t pcb_bxl_sindex[] = {                   6,
    6,    0, -241,    0,    0, -236, -230, -209, -221,    0,
    6,    0,    0,    0,    0,    0,    0,    6, -260, -241,
   15,   19,    0, -253,    0, -198,    0, -233,    6, -170,
 -184, -217, -217, -217,   36, -217, -162, -162, -162,   50,
 -176,   38,   61,   63,   64, -173,    6,    0, -206,    0,
    0,    0,    0,    0,   15,    0,    0,    0,    0,    0,
    0,    0,   19,   58, -141, -217, -217, -217,    0, -170,
    0,    0,    0, -136,    6,   82,   84,   86,    0,    6,
 -249, -217, -217, -217,    0,    0,    0,    0,    0,    0,
    0,   99,   99, -140,    6,    0,    0,    0,    0,    0,
    0,    0,    0,  101,  102,  104, -124,  107,  108,  111,
  112,  113,  114, -276,    0,    0,    0, -249,    0,    0,
    0, -104, -109, -124, -154,    0, -225,    0, -193,    0,
 -215,    0, -182,    0, -234,    0, -217, -100,  -99,    0,
  121,    0,    0,    0,    0,  -95,  -94,  -93,  -92,  -89,
 -217,    0,  124, -217,  -87,  -88,  129,    0,    0,    0,
  133, -217,    0,    0,    0,  134,  -84,  -83,  -79, -162,
    0,    0,    0,  138, -217, -217, -217,    0,    0,    0,
  139, -217,  -78, -162,    0,    0,    0,  140,  141,    0,
    0,   99,  143,    0,    0,    0,    0,    0,    0,  107,
    0,    0,    0, -217,  108,  142,  111,    0,  -76,    0,
    0,  112,    0,    0,    0,  113,    0,    0,    0,  114,
 -217,    0, -189,    6,    0,    0,    0, -217,    0,    0,
    0,    0,    0,    0, -217,  -71,    0,    0,  147,    0,
    0,    0,    0,  143,    0,
};
static const pcb_bxl_int_t pcb_bxl_rindex[] = {                   9,
    1,    0,  189,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  189,
  180,  181,    0,    0,    0,    0,    0,    0,    0,  -86,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  180,    0,    0,    0,    0,    0,
    0,    0,  181,    0,    0,    0,    0,    0,    0,  -86,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -82,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  182,  182,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  -74,  185,  186,  187,
  188,  190,  191,    0,    0,    0,    0,  -82,    0,    0,
    0,    0,    0,  -74,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  182,  192,    0,    0,    0,    0,    0,    0,  185,
    0,    0,    0,    0,  186,    0,  187,    0,    0,    0,
    0,  188,    0,    0,    0,  190,    0,    0,    0,  191,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  192,    0,
};
static const pcb_bxl_int_t pcb_bxl_gindex[] = {                   0,
  -27,  -64,  -26,    0,  183,    0,   -1,    0,    0,    0,
    0,   69,   -4, -108, -118,  150,    0,    0,  144,    0,
   85,    0,    0,    0,  -38,    0,    0,  145,    0,    0,
    0,   90,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   10,    0,    0,    7,    0,    0,    4,    0,    0,
    2,    0,    0,   -3,    0,    0,   -2,    0,    0,  -85,
    0,
};
#define pcb_bxl_TABLESIZE 321
static const pcb_bxl_int_t pcb_bxl_table[] = {                    5,
   10,   76,   77,   78,   52,   53,   54,  116,   12,   20,
  165,   61,   62,  137,  180,    1,   23,  104,  105,  106,
  164,    6,  173,   16,  179,    7,  187,   41,  167,   17,
  138,  139,   50,   51,   19,   86,    8,   36,   37,   38,
   50,   51,  155,   87,   88,   70,   89,  167,  154,   90,
   18,  155,   24,   91,   26,  137,   92,  182,   28,   93,
   31,  155,  157,  169,  137,   32,   33,   34,  183,  170,
    9,   49,  189,   81,  137,  184,   55,   39,   85,  156,
  154,  168,  169,  155,  154,  235,  236,  155,  170,  201,
   63,  154,   64,  118,  155,   65,  137,  206,   58,   59,
   66,  162,   67,   68,  238,   69,  222,  137,   71,   42,
  213,   43,   44,   45,  237,   74,   75,  175,  176,  177,
  152,   80,  158,  199,  163,   82,  172,   83,  178,   84,
  186,  146,  147,  148,  149,  137,  150,  151,  114,  226,
  117,  119,  120,  211,  121,  122,  125,  127,  214,  215,
  129,  131,  133,  135,  217,  143,  234,  219,  144,  190,
  191,  192,  194,  241,  200,  195,  196,  197,  198,  202,
  242,  203,  204,  205,  207,  208,  209,  210,  212,  216,
  220,  218,  223,  230,  221,  228,  243,  244,    2,   25,
   33,  126,   51,   39,   70,   81,   90,   98,   58,  106,
  116,   43,   25,  185,   72,  245,   73,  142,  145,  225,
  229,  227,  232,  231,   79,    0,    0,  233,    0,    0,
    0,    0,  240,    0,    0,    0,    0,    0,    0,    0,
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
static const pcb_bxl_int_t pcb_bxl_check[] = {                    1,
    0,   66,   67,   68,   32,   33,   34,   93,    0,   11,
  129,   38,   39,  290,  133,   10,   18,   82,   83,   84,
  129,  263,  131,  260,  133,  267,  135,   29,  263,  260,
  307,  308,  258,  259,  256,  285,  278,  271,  272,  273,
  258,  259,  277,  293,  294,   47,  296,  263,  274,  299,
  260,  277,  313,  303,   40,  290,  306,  292,   40,  309,
  314,  277,  127,  298,  290,  264,  265,  266,  303,  304,
  312,  256,  137,   75,  290,  310,   41,  311,   80,  305,
  274,  297,  298,  277,  274,  275,  276,  277,  304,  154,
   41,  274,  269,   95,  277,   58,  290,  162,  261,  262,
   40,  295,   40,   40,  223,  279,  192,  290,  315,  280,
  175,  282,  283,  284,  223,   58,  258,  300,  301,  302,
  125,  258,  127,  151,  129,   44,  131,   44,  133,   44,
  135,  286,  287,  288,  289,  290,  291,  292,   40,  204,
  281,   41,   41,  170,   41,  270,   40,   40,  176,  177,
   40,   40,   40,   40,  182,  260,  221,  184,  268,  260,
  260,   41,  258,  228,   41,  260,  260,  260,  258,  257,
  235,  260,   44,   41,   41,  260,  260,  257,   41,   41,
   41,  260,   40,  260,   44,   44,  258,   41,    0,   10,
   10,   10,  279,  268,   10,   10,   10,   10,  281,   10,
   10,   10,   20,  135,   55,  244,   63,  118,  124,  200,
  207,  205,  216,  212,   70,   -1,   -1,  220,   -1,   -1,
   -1,   -1,  224,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
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
#define pcb_bxl_UNDFTOKEN 379
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
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"illegal-symbol",
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
"$$2 :",
"$$3 :",
"pad_stack : T_PADSTACK T_QSTR $$2 pstk_attrs nl T_SHAPES ':' T_INTEGER nl $$3 pad_shapes T_ENDPADSTACK",
"pstk_attrs :",
"pstk_attrs : '(' pstk_attr ')' pstk_attrs",
"pstk_attr : T_HOLEDIAM coord",
"pstk_attr : T_SURFACE boolean",
"pstk_attr : T_PLATED boolean",
"pstk_attr : T_NOPASTE boolean",
"pad_shapes :",
"pad_shapes : pad_shape pad_shapes",
"$$4 :",
"pad_shape : T_PADSHAPE T_QSTR $$4 padshape_attrs nl",
"padshape_attrs :",
"padshape_attrs : '(' padshape_attr ')' padshape_attrs",
"padshape_attr : T_HEIGHT coord",
"padshape_attr : T_PADTYPE T_INTEGER",
"padshape_attr : common_layer",
"padshape_attr : common_width",
"$$5 :",
"pattern : T_PATTERN T_QSTR nl $$5 pattern_chldrn T_ENDPATTERN",
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
"$$6 :",
"pad : T_PAD $$6 pad_attrs",
"pad_attrs :",
"pad_attrs : '(' pad_attr ')' pad_attrs",
"pad_attr : T_NUMBER T_INTEGER",
"pad_attr : T_PINNAME T_QSTR",
"pad_attr : T_PADSTYLE T_QSTR",
"pad_attr : T_ORIGINALPADSTYLE T_QSTR",
"pad_attr : T_ORIGINALPINNUMBER T_INTEGER",
"pad_attr : T_ROTATE real",
"pad_attr : common_origin",
"$$7 :",
"poly : T_POLY $$7 poly_attrs",
"poly_attrs :",
"poly_attrs : '(' poly_attr ')' poly_attrs",
"poly_attr : T_PROPERTY T_QSTR",
"poly_attr : coord ',' coord",
"poly_attr : common_origin",
"poly_attr : common_layer",
"poly_attr : common_width",
"$$8 :",
"line : T_LINE $$8 line_attrs",
"line_attrs :",
"line_attrs : '(' line_attr ')' line_attrs",
"line_attr : T_ENDPOINT coord ',' coord",
"line_attr : common_origin",
"line_attr : common_layer",
"line_attr : common_width",
"$$9 :",
"attribute : T_ATTRIBUTE $$9 attribute_attrs",
"attribute_attrs :",
"attribute_attrs : '(' attribute_attr ')' attribute_attrs",
"attribute_attr : T_ATTR T_QSTR T_QSTR",
"attribute_attr : common_attr_text",
"attribute_attr : common_origin",
"attribute_attr : common_layer",
"$$10 :",
"arc : T_ARC $$10 arc_attrs",
"arc_attrs :",
"arc_attrs : '(' arc_attr ')' arc_attrs",
"arc_attr : T_RADIUS coord",
"arc_attr : T_STARTANGLE real",
"arc_attr : T_SWEEPANGLE real",
"arc_attr : common_origin",
"arc_attr : common_layer",
"arc_attr : common_width",
"$$11 :",
"text : T_TEXT $$11 text_attrs",
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
#line 104 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.i = 1; }
break;
case 9:
#line 105 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.i = 0; }
break;
case 14:
#line 119 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.d = yyctx->stack.l_mark[0].un.i; }
break;
case 15:
#line 120 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.d = yyctx->stack.l_mark[0].un.d; }
break;
case 16:
#line 124 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.c = PCB_MIL_TO_COORD(yyctx->stack.l_mark[0].un.d); }
break;
case 17:
#line 128 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_justify(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 18:
#line 129 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_text_style(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 19:
#line 130 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.is_visible = yyctx->stack.l_mark[0].un.i; }
break;
case 20:
#line 134 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.origin_x = XCRD(yyctx->stack.l_mark[-2].un.c); ctx->state.origin_y = YCRD(yyctx->stack.l_mark[0].un.c); }
break;
case 21:
#line 138 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_layer(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 22:
#line 142 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.width = yyctx->stack.l_mark[0].un.c; }
break;
case 23:
#line 148 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_text_style_begin(ctx, yyctx->stack.l_mark[0].un.s); /* $2 is taken over */ }
break;
case 24:
#line 149 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_text_style_end(ctx); }
break;
case 27:
#line 158 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.text_style->width = yyctx->stack.l_mark[0].un.d; }
break;
case 28:
#line 159 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.text_style->char_width = yyctx->stack.l_mark[0].un.d; }
break;
case 29:
#line 160 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.text_style->height = yyctx->stack.l_mark[0].un.d; }
break;
case 30:
#line 167 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); pcb_bxl_padstack_begin(ctx, yyctx->stack.l_mark[0].un.s); /* $2 is taken over */ }
break;
case 31:
#line 169 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.num_shapes = yyctx->stack.l_mark[-1].un.i; }
break;
case 32:
#line 171 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_padstack_end(ctx); }
break;
case 35:
#line 180 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.hole = yyctx->stack.l_mark[0].un.c; }
break;
case 36:
#line 181 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.surface = yyctx->stack.l_mark[0].un.i; }
break;
case 37:
#line 182 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.plated = yyctx->stack.l_mark[0].un.i; }
break;
case 38:
#line 183 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.nopaste = yyctx->stack.l_mark[0].un.i; }
break;
case 41:
#line 192 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_padstack_begin_shape(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 42:
#line 193 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_padstack_end_shape(ctx); }
break;
case 45:
#line 202 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.height = yyctx->stack.l_mark[0].un.c; }
break;
case 46:
#line 203 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.pad_type = yyctx->stack.l_mark[0].un.i; }
break;
case 49:
#line 210 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_begin(ctx, yyctx->stack.l_mark[-1].un.s); free(yyctx->stack.l_mark[-1].un.s); }
break;
case 50:
#line 212 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_end(ctx); }
break;
case 68:
#line 252 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pad_begin(ctx); }
break;
case 69:
#line 253 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pad_end(ctx); }
break;
case 72:
#line 262 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.pin_number = yyctx->stack.l_mark[0].un.i; }
break;
case 73:
#line 263 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.pin_name = yyctx->stack.l_mark[0].un.s; /* takes over $2 */ }
break;
case 74:
#line 264 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pad_set_style(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 75:
#line 265 "../../src_plugins/io_bxl/bxl_gram.y"
	{ /* what's this?! */ free(yyctx->stack.l_mark[0].un.s); }
break;
case 76:
#line 266 "../../src_plugins/io_bxl/bxl_gram.y"
	{ /* what's this?! */ }
break;
case 77:
#line 267 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.rot = yyctx->stack.l_mark[0].un.d; }
break;
case 79:
#line 273 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_begin(ctx); }
break;
case 80:
#line 274 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_end(ctx); }
break;
case 83:
#line 284 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_property(ctx, (pcb_any_obj_t *)ctx->state.poly, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 84:
#line 285 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_add_vertex(ctx, XCRD(yyctx->stack.l_mark[-2].un.c), YCRD(yyctx->stack.l_mark[0].un.c)); }
break;
case 88:
#line 293 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 89:
#line 294 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_line(ctx); pcb_bxl_reset(ctx); }
break;
case 92:
#line 303 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.endp_x = XCRD(yyctx->stack.l_mark[-2].un.c); ctx->state.endp_y = YCRD(yyctx->stack.l_mark[0].un.c); }
break;
case 96:
#line 311 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); ctx->state.is_text = 0; }
break;
case 97:
#line 312 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_text(ctx); pcb_bxl_reset(ctx); }
break;
case 100:
#line 321 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_attr_val(ctx, yyctx->stack.l_mark[-1].un.s, yyctx->stack.l_mark[0].un.s); /* $2 and $3 are taken over */ }
break;
case 104:
#line 330 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 105:
#line 331 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_arc(ctx); pcb_bxl_reset(ctx); }
break;
case 108:
#line 340 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.radius = yyctx->stack.l_mark[0].un.c;  }
break;
case 109:
#line 341 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.arc_start = yyctx->stack.l_mark[0].un.d; }
break;
case 110:
#line 342 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.arc_delta = yyctx->stack.l_mark[0].un.d; }
break;
case 114:
#line 350 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); ctx->state.is_text = 1; }
break;
case 115:
#line 351 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_text(ctx); pcb_bxl_reset(ctx); }
break;
case 118:
#line 360 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_text_str(ctx, yyctx->stack.l_mark[0].un.s); /* $2 is taken over */ }
break;
case 119:
#line 361 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.flipped = yyctx->stack.l_mark[0].un.i; }
break;
case 120:
#line 362 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.rot = yyctx->stack.l_mark[0].un.d; }
break;
#line 827 "../../src_plugins/io_bxl/bxl_gram.c"
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
