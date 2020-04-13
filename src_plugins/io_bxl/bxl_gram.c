
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
    0,    4,    4,    5,    5,    5,    5,   11,   11,    6,
    6,    3,    3,    1,    1,    2,   12,   12,   12,   13,
   14,    7,   15,   15,   16,   16,   16,    8,   17,   17,
   19,   19,   19,   19,   18,   18,   20,   21,   21,   22,
   22,   22,   22,   24,    9,   23,   23,   25,   25,   25,
   25,   26,   27,   27,   28,   28,   28,   28,   28,   28,
   28,   28,   29,   37,   37,   38,   38,   38,   38,   38,
   38,   38,   40,   30,   39,   39,   41,   41,   41,   41,
   41,   43,   31,   42,   42,   44,   44,   44,   44,   32,
   45,   45,   46,   46,   46,   46,   48,   34,   47,   47,
   49,   49,   49,   49,   49,   49,   35,   50,   50,   51,
   51,   51,   51,   51,   51,   36,   33,   52,   52,   53,
   53,   53,   10,
};
static const pcb_bxl_int_t pcb_bxl_len[] = {                      2,
    2,    0,    3,    1,    1,    1,    1,    1,    1,    1,
    2,    0,    1,    1,    1,    1,    2,    2,    2,    4,
    2,    3,    0,    4,    2,    2,    2,   10,    0,    4,
    2,    2,    2,    2,    0,    2,    4,    0,    4,    2,
    2,    2,    1,    0,    6,    0,    3,    1,    6,    6,
    6,    6,    0,    3,    1,    1,    1,    1,    1,    1,
    1,    1,    2,    0,    4,    2,    2,    2,    2,    2,
    2,    1,    0,    3,    0,    4,    2,    2,    3,    1,
    1,    0,    3,    0,    4,    4,    2,    1,    1,    2,
    0,    4,    3,    1,    1,    1,    0,    3,    0,    4,
    2,    2,    2,    2,    1,    1,    2,    0,    4,    2,
    2,    2,    1,    1,    1,    2,    2,    0,    4,    2,
    2,    1,    6,
};
static const pcb_bxl_int_t pcb_bxl_defred[] = {                   0,
    0,    0,    0,   13,   11,    0,    0,    0,    0,    1,
    0,    4,    5,    6,    7,    0,    0,    0,    0,    0,
    0,   22,    0,    0,   44,    0,    3,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   25,
   26,   27,    0,   31,    8,    9,   32,   33,   34,    0,
    0,    0,    0,    0,    0,    0,    0,   48,    0,   24,
   30,    0,    0,    0,    0,    0,   45,    0,  123,    0,
    0,   14,   15,   16,    0,    0,    0,   47,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   73,   82,    0,
   97,    0,    0,    0,    0,    0,   55,   56,   57,   58,
   59,   60,   61,   62,    0,    0,    0,    0,   28,   36,
    0,   63,    0,    0,    0,   90,    0,    0,  107,    0,
  116,  117,   52,    0,   49,   50,   51,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   72,    0,    0,   74,
    0,   83,    0,    0,    0,    0,    0,   94,   95,   96,
    0,    0,   98,    0,    0,    0,  113,  114,  115,    0,
    0,    0,  122,    0,   54,    0,    0,    0,   43,    0,
   37,   66,   67,   68,   69,    0,   70,   71,    0,    0,
    0,    0,   80,   81,    0,    0,    0,   88,   89,    0,
   18,   21,    0,   17,   19,    0,    0,    0,    0,    0,
  105,  106,    0,  112,  110,  111,    0,  120,  121,    0,
   40,   41,   42,    0,    0,   65,   77,   78,    0,    0,
   87,    0,    0,   93,   92,  101,  102,  103,  104,    0,
  109,  119,   39,   20,   79,   76,    0,   85,  100,   86,
};
static const pcb_bxl_int_t pcb_bxl_dgoto[] = {                    2,
   74,   75,    3,   10,   11,    4,   12,   13,   14,   15,
   47,  148,  137,  150,   22,   31,   24,   85,   36,   86,
  129,  170,   56,   38,   57,   58,   95,   96,   97,   98,
   99,  100,  101,  102,  103,  104,  112,  138,  140,  113,
  185,  142,  114,  190,  116,  151,  153,  117,  203,  119,
  160,  121,  164,
};
static const pcb_bxl_int_t pcb_bxl_sindex[] = {                  -3,
   -3,    0, -251,    0,    0, -230, -224, -221, -235,    0,
   -3,    0,    0,    0,    0,    4,   12,   -3, -273, -251,
 -127,    0, -258,   -3,    0, -260,    0, -194, -186, -185,
   25, -179, -236, -236, -236,   27, -189, -191, -170,    0,
    0,    0,    4,    0,    0,    0,    0,    0,    0,   12,
   38,   39,   59,   62,   63, -175,   -3,    0, -209,    0,
    0, -151, -147, -225, -225, -225,    0, -191,    0,   -3,
   -3,    0,    0,    0,   72,   75,   77,    0, -145, -163,
 -225, -225, -225, -134, -139, -145,   92,    0,    0,  101,
    0,  102,  104,  104, -136,   -3,    0,    0,    0,    0,
    0,    0,    0,    0,  106,  107,  108,  110,    0,    0,
 -129,    0,  111,  113, -239,    0,  114, -255,    0, -262,
    0,    0,    0, -163,    0,    0,    0, -162,   -3, -103,
  -96,  -95,  -94, -225,  -91, -225,    0,  127, -227,    0,
 -190,    0,  -89,  -88,  -87,  -85, -236,    0,    0,    0,
  129, -192,    0, -225,  -84, -236,    0,    0,    0,  134,
  -83,  -82,    0,  138,    0, -225, -225,  -78,    0,  140,
    0,    0,    0,    0,    0,  139,    0,    0,   92, -225,
  -76,  141,    0,    0,  145, -225, -225,    0,    0,  146,
    0,    0,  -72,    0,    0,  101, -225, -225, -225, -225,
    0,    0,  148,    0,    0,    0,  102,    0,    0,  104,
    0,    0,    0,  110, -225,    0,    0,    0, -225,  111,
    0,  147,  113,    0,    0,    0,    0,    0,    0,  114,
    0,    0,    0,    0,    0,    0, -225,    0,    0,    0,
};
static const pcb_bxl_int_t pcb_bxl_rindex[] = {                   9,
    1,    0,  182,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  180,  183,    0,    0,  182,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -81,    0,    0,
    0,    0,  180,    0,    0,    0,    0,    0,    0,  183,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -81,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -74,  -86,
    0,    0,    0,    0,    0,  -74,  186,    0,    0,  187,
    0,  189,  190,  190,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  191,    0,    0,
    0,    0,  192,  193,    0,    0,  194,    0,    0,    0,
    0,    0,    0,  -86,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  186,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  187,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  189,    0,    0,  190,
    0,    0,    0,  191,    0,    0,    0,    0,    0,  192,
    0,    0,  193,    0,    0,    0,    0,    0,    0,  194,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
static const pcb_bxl_int_t pcb_bxl_gindex[] = {                   0,
 -125,  -63,    0,  172,    0,   -1,    0,    0,    0,    0,
  -29,   87,  -58,  -51,  163,    0,  157,  122,    0,    0,
   -5,    0,  142,    0,    0,    0,   88,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   32,    0,   -7,    0,
    0,   -9,    0,    0,   19,    0,  -14,    0,    0,   10,
    0,  -90,    0,
};
#define pcb_bxl_TABLESIZE 321
static const pcb_bxl_int_t pcb_bxl_table[] = {                    5,
   10,   76,   77,  122,   48,   49,    1,  143,   12,   20,
  178,    6,   32,   33,   34,    7,   25,  105,  106,  107,
   19,  144,   37,  143,   45,   46,    8,  134,  204,   16,
   72,   73,   72,   73,  134,   17,  154,  144,   18,   26,
  211,  212,  146,   21,  161,  162,  180,  155,  147,  144,
  134,   23,   35,   39,  156,   68,  149,  145,  146,  158,
    9,  163,  134,   40,  147,   43,  159,   50,   79,   80,
  176,   41,   42,  228,  229,  182,  169,  181,   44,   51,
  183,  197,  188,  186,  144,   59,  144,  184,   52,  189,
   53,   54,   55,  201,  124,   62,   63,  134,   64,  134,
  202,   65,   66,   67,  187,   69,   70,  198,  199,  200,
   71,  166,  167,  168,  144,   81,  217,  195,   82,  232,
   83,   87,  221,  222,   84,  108,  206,  171,  109,   88,
   89,  111,   90,  226,  227,   91,   28,   29,   30,   92,
  115,  118,   93,  120,  123,   94,  125,  126,  127,  128,
  139,  234,  141,  152,  172,  235,  130,  131,  132,  133,
  134,  135,  136,  173,  174,  175,  177,  179,  192,  196,
  191,  194,  193,  240,  207,  205,  208,  209,  210,  213,
  214,    2,  215,  218,  219,  220,  223,  224,  230,   23,
  237,   27,   29,   35,   53,   64,   91,   46,  108,  118,
   38,   75,   84,   99,  157,   60,   61,  110,  233,   78,
  216,  165,  236,  238,  225,  239,  231,    0,    0,    0,
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
static const pcb_bxl_int_t pcb_bxl_check[] = {                    1,
    0,   65,   66,   94,   34,   35,   10,  263,    0,   11,
  136,  263,  271,  272,  273,  267,   18,   81,   82,   83,
  256,  277,   24,  263,  261,  262,  278,  290,  154,  260,
  258,  259,  258,  259,  290,  260,  292,  277,  260,  313,
  166,  167,  298,   40,  307,  308,  274,  303,  304,  277,
  290,   40,  311,  314,  310,   57,  115,  297,  298,  118,
  312,  120,  290,  258,  304,   41,  118,   41,   70,   71,
  134,  258,  258,  199,  200,  139,  128,  305,  258,  269,
  139,  274,  141,  274,  277,  256,  277,  139,  280,  141,
  282,  283,  284,  152,   96,   58,   58,  290,   40,  290,
  152,   40,   40,  279,  295,  315,  258,  300,  301,  302,
  258,  274,  275,  276,  277,   44,  180,  147,   44,  210,
   44,  285,  186,  187,  270,  260,  156,  129,  268,  293,
  294,   40,  296,  197,  198,  299,  264,  265,  266,  303,
   40,   40,  306,   40,  281,  309,   41,   41,   41,   40,
   40,  215,   40,   40,  258,  219,  286,  287,  288,  289,
  290,  291,  292,  260,  260,  260,  258,   41,  257,   41,
  260,  257,  260,  237,   41,  260,  260,  260,   41,  258,
   41,    0,   44,  260,   44,   41,   41,  260,   41,   10,
   44,   20,   10,  268,  281,   10,   10,  279,   10,   10,
   10,   10,   10,   10,  118,   43,   50,   86,  214,   68,
  179,  124,  220,  223,  196,  230,  207,   -1,   -1,   -1,
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
#define pcb_bxl_UNDFTOKEN 371
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
0,0,0,0,0,0,0,0,0,"illegal-symbol",
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
"padshape_attr : common_layer",
"$$1 :",
"pattern : T_PATTERN T_QSTR nl $$1 pattern_chldrn T_ENDPATTERN",
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
"$$2 :",
"poly : T_POLY $$2 poly_attrs",
"poly_attrs :",
"poly_attrs : '(' poly_attr ')' poly_attrs",
"poly_attr : T_WIDTH coord",
"poly_attr : T_PROPERTY T_QSTR",
"poly_attr : coord ',' coord",
"poly_attr : common_origin",
"poly_attr : common_layer",
"$$3 :",
"line : T_LINE $$3 line_attrs",
"line_attrs :",
"line_attrs : '(' line_attr ')' line_attrs",
"line_attr : T_ENDPOINT coord ',' coord",
"line_attr : T_WIDTH coord",
"line_attr : common_origin",
"line_attr : common_layer",
"attribute : T_ATTRIBUTE attribute_attrs",
"attribute_attrs :",
"attribute_attrs : '(' attribute_attr ')' attribute_attrs",
"attribute_attr : T_ATTR T_QSTR T_QSTR",
"attribute_attr : common_attr_text",
"attribute_attr : common_origin",
"attribute_attr : common_layer",
"$$4 :",
"arc : T_ARC $$4 arc_attrs",
"arc_attrs :",
"arc_attrs : '(' arc_attr ')' arc_attrs",
"arc_attr : T_WIDTH coord",
"arc_attr : T_RADIUS coord",
"arc_attr : T_STARTANGLE real",
"arc_attr : T_SWEEPANGLE real",
"arc_attr : common_origin",
"arc_attr : common_layer",
"text : T_TEXT text_attrs",
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
case 14:
#line 115 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.d = yyctx->stack.l_mark[0].un.i; }
break;
case 15:
#line 116 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.d = yyctx->stack.l_mark[0].un.d; }
break;
case 16:
#line 120 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.c = PCB_MIL_TO_COORD(yyctx->stack.l_mark[0].un.d); }
break;
case 20:
#line 130 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_ORIGIN_X, yyctx->stack.l_mark[-2].un.c); pcb_bxl_set_coord(ctx, BXL_ORIGIN_Y, yyctx->stack.l_mark[0].un.c); }
break;
case 21:
#line 134 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_layer(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 44:
#line 199 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_begin(ctx, yyctx->stack.l_mark[-1].un.s); free(yyctx->stack.l_mark[-1].un.s); }
break;
case 45:
#line 201 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_end(ctx); }
break;
case 73:
#line 261 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_begin(ctx); }
break;
case 74:
#line 262 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_end(ctx); }
break;
case 77:
#line 271 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_WIDTH, yyctx->stack.l_mark[0].un.c); }
break;
case 78:
#line 272 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_property(ctx, (pcb_any_obj_t *)ctx->state.poly, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 79:
#line 273 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_add_vertex(ctx, yyctx->stack.l_mark[-2].un.c, yyctx->stack.l_mark[0].un.c); }
break;
case 82:
#line 280 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 83:
#line 281 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_line(ctx); pcb_bxl_reset(ctx); }
break;
case 86:
#line 290 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_ENDP_X, yyctx->stack.l_mark[-2].un.c); pcb_bxl_set_coord(ctx, BXL_ENDP_Y, yyctx->stack.l_mark[0].un.c); }
break;
case 87:
#line 291 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_WIDTH, yyctx->stack.l_mark[0].un.c); }
break;
case 97:
#line 316 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 98:
#line 317 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_arc(ctx); pcb_bxl_reset(ctx); }
break;
case 102:
#line 327 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_RADIUS, yyctx->stack.l_mark[0].un.c); }
break;
case 103:
#line 328 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.arc_start = yyctx->stack.l_mark[0].un.d; }
break;
case 104:
#line 329 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.arc_delta = yyctx->stack.l_mark[0].un.d; }
break;
#line 668 "../../src_plugins/io_bxl/bxl_gram.c"
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
