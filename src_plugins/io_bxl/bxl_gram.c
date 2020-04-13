
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
   33,   33,   33,   33,   33,   33,   33,   34,   42,   42,
   43,   43,   43,   43,   43,   43,   43,   45,   35,   44,
   44,   46,   46,   46,   46,   46,   48,   36,   47,   47,
   49,   49,   49,   49,   37,   50,   50,   51,   51,   51,
   51,   53,   39,   52,   52,   54,   54,   54,   54,   54,
   54,   56,   40,   55,   55,   57,   57,   57,   57,   57,
   57,   41,   38,   58,   58,   59,   59,   59,   11,
};
static const pcb_bxl_int_t pcb_bxl_len[] = {                      2,
    2,    0,    3,    1,    1,    1,    1,    1,    1,    1,
    2,    0,    1,    1,    1,    1,    2,    2,    2,    4,
    2,    2,    0,    4,    0,    4,    2,    2,    2,    0,
    0,   12,    0,    4,    2,    2,    2,    2,    0,    2,
    0,    5,    0,    4,    2,    2,    1,    1,    0,    6,
    0,    3,    1,    6,    6,    6,    6,    0,    3,    1,
    1,    1,    1,    1,    1,    1,    1,    2,    0,    4,
    2,    2,    2,    2,    2,    2,    1,    0,    3,    0,
    4,    2,    3,    1,    1,    1,    0,    3,    0,    4,
    4,    1,    1,    1,    2,    0,    4,    3,    1,    1,
    1,    0,    3,    0,    4,    2,    2,    2,    1,    1,
    1,    0,    3,    0,    4,    2,    2,    2,    1,    1,
    1,    2,    2,    0,    4,    2,    2,    1,    6,
};
static const pcb_bxl_int_t pcb_bxl_defred[] = {                   0,
    0,    0,    0,   13,   11,    0,    0,    0,    0,    1,
    0,    4,    5,    6,    7,   23,   30,    0,    0,    0,
    0,    0,   49,    0,    3,    0,   24,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   53,    0,   14,
   15,   27,   28,   29,    0,   16,   35,    8,    9,   36,
   37,   38,    0,    0,    0,    0,    0,    0,   50,    0,
  129,   26,   34,    0,    0,    0,    0,    0,   52,    0,
    0,    0,    0,    0,   31,    0,   78,   87,    0,  102,
  112,    0,    0,    0,    0,   60,   61,   62,   63,   64,
   65,   66,   67,    0,    0,    0,    0,    0,   68,    0,
    0,    0,   95,    0,    0,    0,  122,  123,   57,    0,
   54,   55,   56,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   77,    0,    0,   79,    0,   88,    0,
    0,    0,    0,    0,   99,  100,  101,    0,    0,  103,
    0,  113,    0,    0,  128,    0,   59,   41,   32,   40,
   71,   72,   73,   74,    0,   75,   76,    0,    0,    0,
    0,   84,   85,   86,    0,    0,   92,   93,   94,    0,
   18,   21,    0,   17,   19,    0,    0,    0,    0,  109,
  110,  111,    0,    0,    0,    0,  119,  120,  121,    0,
  126,  127,    0,    0,    0,   70,   22,   82,    0,    0,
    0,    0,   98,   97,  106,  107,  108,    0,  118,  116,
  117,    0,  125,    0,    0,   20,   83,   81,    0,   90,
  105,  115,    0,    0,   47,   48,    0,   42,   91,   45,
   46,    0,   44,
};
static const pcb_bxl_int_t pcb_bxl_dgoto[] = {                    2,
   56,   57,   60,    3,   10,   11,    4,   12,   13,   14,
   15,  145,  134,  147,  174,   27,   21,   35,   29,   22,
  125,  107,   40,  126,  225,  204,  237,   46,   30,   47,
   48,   94,   95,   96,   97,   98,   99,  100,  101,  102,
  103,  109,  135,  137,  110,  175,  139,  111,  180,  113,
  148,  150,  114,  193,  152,  115,  200,  117,  156,
};
static const pcb_bxl_int_t pcb_bxl_sindex[] = {                  13,
   13,    0, -242,    0,    0, -230, -198, -194, -195,    0,
   13,    0,    0,    0,    0,    0,    0,   13, -245, -242,
   29,   31,    0, -233,    0, -212,    0, -231,   13, -207,
 -173, -225, -225, -225,   54, -225, -217, -217, -217,   55,
 -169,   45,   64,   67,   77, -155,   13,    0, -189,    0,
    0,    0,    0,    0,   29,    0,    0,    0,    0,    0,
    0,    0,   31,   70, -127, -225, -225, -225,    0, -207,
    0,    0,    0, -125,   13,   90,   92,   93,    0,   13,
 -174, -225, -225, -225,    0,   98,    0,    0,   99,    0,
    0,  100,  100, -139,   13,    0,    0,    0,    0,    0,
    0,    0,    0,  103,  105,  106, -121, -199,    0,  110,
  111, -239,    0,  112,  113, -251,    0,    0,    0, -174,
    0,    0,    0, -106, -113, -121, -102, -103, -101, -100,
 -225,  -95, -225,    0,  117, -227,    0, -147,    0,  -96,
  -91,  -92,  -86, -217,    0,    0,    0,  131, -192,    0,
 -255,    0,  -87,  -85,    0,  133,    0,    0,    0,    0,
    0,    0,    0,    0,  132,    0,    0,   98, -225,  -83,
  134,    0,    0,    0,  138, -225,    0,    0,    0,  139,
    0,    0,  -79,    0,    0,   99, -225, -225, -225,    0,
    0,    0,  141, -225,  -77, -217,    0,    0,    0,  143,
    0,    0,  100,  145, -225,    0,    0,    0, -225,  110,
  142,  111,    0,    0,    0,    0,    0,  112,    0,    0,
    0,  113,    0, -161,   13,    0,    0,    0, -225,    0,
    0,    0, -225,  -71,    0,    0,  147,    0,    0,    0,
    0,  145,    0,
};
static const pcb_bxl_int_t pcb_bxl_rindex[] = {                   9,
    1,    0,  189,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  189,
  180,  181,    0,    0,    0,    0,    0,    0,    0,  -84,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  180,    0,    0,    0,    0,    0,
    0,    0,  181,    0,    0,    0,    0,    0,    0,  -84,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -89,    0,    0,    0,    0,  183,    0,    0,  184,    0,
    0,  186,  186,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  -70,    0,    0,  187,
  190,    0,    0,  191,  192,    0,    0,    0,    0,  -89,
    0,    0,    0,    0,    0,  -70,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  183,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  184,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  186,  193,    0,    0,    0,    0,    0,  187,
    0,  190,    0,    0,    0,    0,    0,  191,    0,    0,
    0,  192,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  193,    0,
};
static const pcb_bxl_int_t pcb_bxl_gindex[] = {                   0,
  -27,  -64,  -26,    0,  179,    0,   -1,    0,    0,    0,
    0,   53,  -52, -122, -123,  150,    0,    0,  144,    0,
   80,    0,    0,    0,  -34,    0,    0,  140,    0,    0,
    0,   89,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   43,    0,    2,    0,    0,    3,    0,    0,   27,
    0,   -4,    0,    0,   -6,    0,    0,  -82,    0,
};
#define pcb_bxl_TABLESIZE 321
static const pcb_bxl_int_t pcb_bxl_table[] = {                    5,
   10,   76,   77,   78,   52,   53,   54,  140,   12,   20,
  118,   61,   62,  173,  179,  178,   23,  104,  105,  106,
    6,  141,    1,  140,    7,  192,  191,   41,  199,   16,
   50,   51,   50,   51,  131,    8,  194,  141,  131,   36,
   37,   38,  143,   58,   59,   70,  169,  195,  144,  141,
  131,   32,   33,   34,  196,  153,  154,  142,  143,  146,
   19,   17,  131,  155,  144,   18,  165,   24,   26,    9,
   28,  171,   42,   81,   43,   44,   45,  170,   85,   39,
   31,  169,   49,  172,  141,  177,  127,  128,  129,  130,
  131,  132,  133,  120,   55,   63,  190,  131,  198,   64,
  236,  235,   65,   66,  207,  167,   67,  187,  188,  189,
   86,  211,  169,  233,  234,  141,   68,  185,   87,   88,
  223,   89,  215,   69,   90,   71,  169,   74,   91,  141,
   75,   92,   80,   82,   93,   83,   84,  108,  112,  116,
  226,  119,  131,  121,  227,  122,  123,  176,  124,  136,
  138,  149,  151,  158,  159,  161,  162,  168,  163,  164,
  216,  217,  166,  181,  239,  182,  219,  183,  240,  221,
  184,  186,  201,  203,  202,  205,  208,  209,  210,  212,
  213,  218,  220,  222,  224,  229,  241,  242,    2,   25,
   33,   58,   69,   96,   51,  124,   80,   39,   25,   89,
  104,  114,   43,  197,   72,  160,   73,  243,  157,   79,
  206,  228,  214,  231,  230,  232,    0,    0,    0,    0,
    0,    0,    0,  238,    0,    0,    0,    0,    0,    0,
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
    0,   66,   67,   68,   32,   33,   34,  263,    0,   11,
   93,   38,   39,  136,  138,  138,   18,   82,   83,   84,
  263,  277,   10,  263,  267,  149,  149,   29,  151,  260,
  258,  259,  258,  259,  290,  278,  292,  277,  290,  271,
  272,  273,  298,  261,  262,   47,  274,  303,  304,  277,
  290,  264,  265,  266,  310,  307,  308,  297,  298,  112,
  256,  260,  290,  116,  304,  260,  131,  313,   40,  312,
   40,  136,  280,   75,  282,  283,  284,  305,   80,  311,
  314,  274,  256,  136,  277,  138,  286,  287,  288,  289,
  290,  291,  292,   95,   41,   41,  149,  290,  151,  269,
  224,  224,   58,   40,  169,  133,   40,  300,  301,  302,
  285,  176,  274,  275,  276,  277,   40,  144,  293,  294,
  203,  296,  187,  279,  299,  315,  274,   58,  303,  277,
  258,  306,  258,   44,  309,   44,   44,   40,   40,   40,
  205,  281,  290,   41,  209,   41,   41,  295,  270,   40,
   40,   40,   40,  260,  268,  258,  260,   41,  260,  260,
  188,  189,  258,  260,  229,  257,  194,  260,  233,  196,
  257,   41,  260,   41,  260,   44,  260,   44,   41,   41,
  260,   41,  260,   41,   40,   44,  258,   41,    0,   10,
   10,  281,   10,   10,  279,   10,   10,  268,   20,   10,
   10,   10,   10,  151,   55,  126,   63,  242,  120,   70,
  168,  210,  186,  218,  212,  222,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  225,   -1,   -1,   -1,   -1,   -1,   -1,
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
#define pcb_bxl_UNDFTOKEN 377
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
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"illegal-symbol",
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
"$$6 :",
"poly : T_POLY $$6 poly_attrs",
"poly_attrs :",
"poly_attrs : '(' poly_attr ')' poly_attrs",
"poly_attr : T_PROPERTY T_QSTR",
"poly_attr : coord ',' coord",
"poly_attr : common_origin",
"poly_attr : common_layer",
"poly_attr : common_width",
"$$7 :",
"line : T_LINE $$7 line_attrs",
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
"$$8 :",
"arc : T_ARC $$8 arc_attrs",
"arc_attrs :",
"arc_attrs : '(' arc_attr ')' arc_attrs",
"arc_attr : T_RADIUS coord",
"arc_attr : T_STARTANGLE real",
"arc_attr : T_SWEEPANGLE real",
"arc_attr : common_origin",
"arc_attr : common_layer",
"arc_attr : common_width",
"$$9 :",
"text : T_TEXT $$9 text_attrs",
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
case 30:
#line 164 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_padstack_begin(ctx, yyctx->stack.l_mark[0].un.s); /* $2 is taken over */ }
break;
case 31:
#line 166 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.num_shapes = yyctx->stack.l_mark[-1].un.i; }
break;
case 32:
#line 168 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_padstack_end(ctx); }
break;
case 35:
#line 177 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.hole = yyctx->stack.l_mark[0].un.c; }
break;
case 36:
#line 178 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.surface = yyctx->stack.l_mark[0].un.i; }
break;
case 37:
#line 179 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.plated = yyctx->stack.l_mark[0].un.i; }
break;
case 38:
#line 180 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.nopaste = yyctx->stack.l_mark[0].un.i; }
break;
case 41:
#line 189 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_padstack_begin_shape(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 42:
#line 190 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_padstack_end_shape(ctx); }
break;
case 45:
#line 199 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.height = yyctx->stack.l_mark[0].un.c; }
break;
case 46:
#line 200 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.pad_type = yyctx->stack.l_mark[0].un.i; }
break;
case 49:
#line 207 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_begin(ctx, yyctx->stack.l_mark[-1].un.s); free(yyctx->stack.l_mark[-1].un.s); }
break;
case 50:
#line 209 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_end(ctx); }
break;
case 78:
#line 269 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_begin(ctx); }
break;
case 79:
#line 270 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_end(ctx); }
break;
case 82:
#line 280 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_property(ctx, (pcb_any_obj_t *)ctx->state.poly, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 83:
#line 281 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_add_vertex(ctx, yyctx->stack.l_mark[-2].un.c, yyctx->stack.l_mark[0].un.c); }
break;
case 87:
#line 289 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 88:
#line 290 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_line(ctx); pcb_bxl_reset(ctx); }
break;
case 91:
#line 299 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.endp_x = yyctx->stack.l_mark[-2].un.c; ctx->state.endp_y = yyctx->stack.l_mark[0].un.c; }
break;
case 102:
#line 325 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 103:
#line 326 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_arc(ctx); pcb_bxl_reset(ctx); }
break;
case 106:
#line 335 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.radius = yyctx->stack.l_mark[0].un.c;  }
break;
case 107:
#line 336 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.arc_start = yyctx->stack.l_mark[0].un.d; }
break;
case 108:
#line 337 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.arc_delta = yyctx->stack.l_mark[0].un.d; }
break;
case 112:
#line 345 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 113:
#line 346 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_text(ctx); pcb_bxl_reset(ctx); }
break;
case 116:
#line 355 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_text_str(ctx, yyctx->stack.l_mark[0].un.s); /* $2 is taken over */ }
break;
case 117:
#line 356 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.flipped = yyctx->stack.l_mark[0].un.i; }
break;
case 118:
#line 357 "../../src_plugins/io_bxl/bxl_gram.y"
	{ ctx->state.rot = yyctx->stack.l_mark[0].un.d; }
break;
#line 777 "../../src_plugins/io_bxl/bxl_gram.c"
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
