
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
    7,   14,   14,   15,   15,   15,    8,   16,   16,   18,
   18,   18,   18,   17,   17,   19,   20,   20,   21,   21,
   21,   21,   23,    9,   22,   22,   24,   24,   24,   24,
   25,   26,   26,   27,   27,   27,   27,   27,   27,   27,
   27,   28,   36,   36,   37,   37,   37,   37,   37,   37,
   37,   39,   29,   38,   38,   40,   40,   40,   40,   40,
   42,   30,   41,   41,   43,   43,   43,   43,   31,   44,
   44,   45,   45,   45,   45,   33,   46,   46,   47,   47,
   47,   47,   47,   47,   34,   48,   48,   49,   49,   49,
   49,   49,   49,   35,   32,   50,   50,   51,   51,   51,
   10,
};
static const pcb_bxl_int_t pcb_bxl_len[] = {                      2,
    2,    0,    3,    1,    1,    1,    1,    1,    1,    1,
    2,    0,    1,    1,    1,    1,    2,    2,    2,    4,
    3,    0,    4,    2,    2,    2,   10,    0,    4,    2,
    2,    2,    2,    0,    2,    4,    0,    4,    2,    2,
    2,    2,    0,    6,    0,    3,    1,    6,    6,    6,
    6,    0,    3,    1,    1,    1,    1,    1,    1,    1,
    1,    2,    0,    4,    2,    2,    2,    2,    2,    2,
    1,    0,    3,    0,    4,    2,    2,    2,    3,    1,
    0,    3,    0,    4,    2,    4,    2,    1,    2,    0,
    4,    2,    3,    1,    1,    2,    0,    4,    2,    2,
    2,    2,    2,    1,    2,    0,    4,    2,    2,    2,
    2,    1,    1,    2,    2,    0,    4,    2,    2,    1,
    6,
};
static const pcb_bxl_int_t pcb_bxl_defred[] = {                   0,
    0,    0,    0,   13,   11,    0,    0,    0,    0,    1,
    0,    4,    5,    6,    7,    0,    0,    0,    0,    0,
    0,   21,    0,    0,   43,    0,    3,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   24,
   25,   26,    0,   30,    8,    9,   31,   32,   33,    0,
    0,    0,    0,    0,    0,    0,    0,   47,    0,   23,
   29,    0,    0,    0,    0,    0,   44,    0,  121,    0,
    0,   14,   15,   16,    0,    0,    0,   46,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   72,   81,    0,
    0,    0,    0,    0,    0,    0,   54,   55,   56,   57,
   58,   59,   60,   61,    0,    0,    0,    0,   27,   35,
    0,   62,    0,    0,    0,   89,    0,   96,    0,  105,
    0,  114,  115,   51,    0,   48,   49,   50,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   71,    0,    0,
   73,    0,   82,    0,    0,    0,    0,    0,   94,   95,
    0,    0,    0,    0,    0,    0,  104,    0,    0,    0,
    0,    0,  112,  113,    0,    0,    0,  120,    0,   53,
    0,    0,    0,    0,    0,   36,   65,   66,   67,   68,
    0,   69,   70,    0,    0,    0,    0,    0,   80,    0,
    0,    0,    0,   88,    0,   18,   92,    0,   17,   19,
    0,  100,   99,  101,  102,  103,    0,  108,  111,  109,
  110,    0,  118,  119,    0,   39,   40,   41,   42,    0,
    0,   64,   77,   76,   78,    0,    0,   87,   85,    0,
    0,   93,   91,   98,  107,  117,   38,   20,   79,   75,
    0,   84,   86,
};
static const pcb_bxl_int_t pcb_bxl_dgoto[] = {                    2,
   74,   75,    3,   10,   11,    4,   12,   13,   14,   15,
   47,  149,  138,   22,   31,   24,   85,   36,   86,  130,
  175,   56,   38,   57,   58,   95,   96,   97,   98,   99,
  100,  101,  102,  103,  104,  112,  139,  141,  113,  190,
  143,  114,  195,  116,  151,  118,  158,  120,  165,  122,
  169,
};
static const pcb_bxl_int_t pcb_bxl_sindex[] = {                  -2,
   -2,    0, -252,    0,    0, -238, -231, -223, -242,    0,
   -2,    0,    0,    0,    0,  -13,   21,   -2, -272, -252,
 -145,    0, -233,   -2,    0, -249,    0, -192, -173, -171,
   52, -159, -237, -237, -237,   61, -165, -186, -151,    0,
    0,    0,  -13,    0,    0,    0,    0,    0,    0,   21,
   49,   50,   72,   74,   75, -162,   -2,    0, -191,    0,
    0, -135, -133, -226, -226, -226,    0, -186,    0,   -2,
   -2,    0,    0,    0,   82,   87,   96,    0, -129, -193,
 -226, -226, -226, -118, -125, -129,  104,    0,    0,  105,
  106,  107,  108,  108, -132,   -2,    0,    0,    0,    0,
    0,    0,    0,    0,  109,  110,  111,  113,    0,    0,
 -153,    0,  114,  115, -247,    0, -219,    0, -256,    0,
 -255,    0,    0,    0, -193,    0,    0,    0, -201,   -2,
 -102, -103, -101, -100, -226,  -97, -226,    0,  121, -246,
    0, -228,    0,  -96,  -91,  -95,  -90, -237,    0,    0,
  127, -226,  -87, -226, -226, -226,    0,  128,  -86, -226,
  -85, -237,    0,    0,  131,  -84,  -83,    0,  132,    0,
 -226, -226,  -79,  -77,  133,    0,    0,    0,    0,    0,
  137,    0,    0,  104, -226,  -75,  -76,  139,    0,  144,
 -226,  -71, -226,    0,  146,    0,    0,  -72,    0,    0,
  105,    0,    0,    0,    0,    0,  106,    0,    0,    0,
    0,  107,    0,    0,  108,    0,    0,    0,    0,  113,
 -226,    0,    0,    0,    0, -226,  114,    0,    0,  145,
  115,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -226,    0,    0,
};
static const pcb_bxl_int_t pcb_bxl_rindex[] = {                   9,
    1,    0,  190,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  181,  182,    0,    0,  190,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -82,    0,    0,
    0,    0,  181,    0,    0,    0,    0,    0,    0,  182,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -82,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -74,  -81,
    0,    0,    0,    0,    0,  -74,  183,    0,    0,  185,
  186,  188,  189,  189,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  191,    0,    0,
    0,    0,  192,  193,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  -81,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  183,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  185,    0,    0,    0,    0,    0,  186,    0,    0,    0,
    0,  188,    0,    0,  189,    0,    0,    0,    0,  191,
    0,    0,    0,    0,    0,    0,  192,    0,    0,    0,
  193,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
static const pcb_bxl_int_t pcb_bxl_gindex[] = {                   0,
  -92,  -63,    0,  184,    0,   -1,    0,    0,    0,    0,
  -30,   86,  -31,  163,    0,  157,  122,    0,    0,  -11,
    0,  142,    0,    0,    0,   88,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   27,    0,  -15,    0,    0,
  -17,    0,    0,   14,    0,   10,    0,    4,    0,  -88,
    0,
};
#define pcb_bxl_TABLESIZE 321
static const pcb_bxl_int_t pcb_bxl_table[] = {                    5,
   10,   76,   77,   48,   49,  123,  144,    1,   12,   20,
    6,   72,   73,   19,    7,  144,   25,  105,  106,  107,
  159,   16,   37,   45,   46,    8,   21,  185,   17,  145,
  186,   72,   73,  135,  135,  160,   18,   32,   33,   34,
   26,  147,  135,  135,  183,  191,  161,  148,  192,  146,
  147,  166,  167,  162,  152,   68,  148,  153,  187,    9,
   23,  135,  205,  206,   39,   40,  193,  209,   79,   80,
  135,  181,  171,  172,  173,  174,  188,   35,  216,  217,
  154,  155,  156,  150,   41,  157,   42,  164,  202,  168,
  204,   87,   43,   52,  125,   53,   54,   55,   44,   88,
   89,   50,   90,   51,   59,   91,   62,   63,  189,   92,
  194,   64,   93,   65,   66,   94,   67,  200,   28,   29,
   30,  223,   70,   69,   71,   81,  236,  228,  176,  230,
   82,  211,  131,  132,  133,  134,  135,  136,  137,   83,
   84,  108,  109,  111,  115,  117,  119,  121,  124,  126,
  127,  128,  129,  140,  142,  177,  178,  238,  179,  180,
  182,  184,  239,  196,  198,  197,  199,  201,  207,  203,
  208,  212,  215,  220,  210,  213,  214,  243,  218,  219,
  221,  224,  226,  225,  227,  229,  231,  232,  241,    2,
   22,   28,   63,   34,   90,   97,   45,  106,  116,   52,
   37,   74,   83,   27,  163,   60,   61,  110,  237,   78,
  222,  240,  170,  242,  233,  235,  234,    0,    0,    0,
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
    0,   65,   66,   34,   35,   94,  263,   10,    0,   11,
  263,  258,  259,  256,  267,  263,   18,   81,   82,   83,
  277,  260,   24,  261,  262,  278,   40,  274,  260,  277,
  277,  258,  259,  290,  290,  292,  260,  271,  272,  273,
  313,  298,  290,  290,  137,  274,  303,  304,  277,  297,
  298,  307,  308,  310,  274,   57,  304,  277,  305,  312,
   40,  290,  155,  156,  314,  258,  295,  160,   70,   71,
  290,  135,  274,  275,  276,  277,  140,  311,  171,  172,
  300,  301,  302,  115,  258,  117,  258,  119,  152,  121,
  154,  285,   41,  280,   96,  282,  283,  284,  258,  293,
  294,   41,  296,  269,  256,  299,   58,   58,  140,  303,
  142,   40,  306,   40,   40,  309,  279,  148,  264,  265,
  266,  185,  258,  315,  258,   44,  215,  191,  130,  193,
   44,  162,  286,  287,  288,  289,  290,  291,  292,   44,
  270,  260,  268,   40,   40,   40,   40,   40,  281,   41,
   41,   41,   40,   40,   40,  258,  260,  221,  260,  260,
  258,   41,  226,  260,  260,  257,  257,   41,   41,  257,
  257,   41,   41,   41,  260,  260,  260,  241,  258,  257,
   44,  257,   44,  260,   41,  257,   41,  260,   44,    0,
   10,   10,   10,  268,   10,   10,  279,   10,   10,  281,
   10,   10,   10,   20,  119,   43,   50,   86,  220,   68,
  184,  227,  125,  231,  201,  212,  207,   -1,   -1,   -1,
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
#define pcb_bxl_UNDFTOKEN 369
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
0,0,0,0,0,0,0,"illegal-symbol",
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
"poly_attr : T_LAYER T_ID",
"poly_attr : T_WIDTH coord",
"poly_attr : T_PROPERTY T_QSTR",
"poly_attr : coord ',' coord",
"poly_attr : common_origin",
"$$3 :",
"line : T_LINE $$3 line_attrs",
"line_attrs :",
"line_attrs : '(' line_attr ')' line_attrs",
"line_attr : T_LAYER T_ID",
"line_attr : T_ENDPOINT coord ',' coord",
"line_attr : T_WIDTH coord",
"line_attr : common_origin",
"attribute : T_ATTRIBUTE attribute_attrs",
"attribute_attrs :",
"attribute_attrs : '(' attribute_attr ')' attribute_attrs",
"attribute_attr : T_LAYER T_ID",
"attribute_attr : T_ATTR T_QSTR T_QSTR",
"attribute_attr : common_attr_text",
"attribute_attr : common_origin",
"arc : T_ARC arc_attrs",
"arc_attrs :",
"arc_attrs : '(' arc_attr ')' arc_attrs",
"arc_attr : T_LAYER T_ID",
"arc_attr : T_WIDTH coord",
"arc_attr : T_RADIUS coord",
"arc_attr : T_STARTANGLE real",
"arc_attr : T_SWEEPANGLE real",
"arc_attr : common_origin",
"text : T_TEXT text_attrs",
"text_attrs :",
"text_attrs : '(' text_attr ')' text_attrs",
"text_attr : T_LAYER T_ID",
"text_attr : T_TEXT T_QSTR",
"text_attr : T_ISFLIPPED boolean",
"text_attr : T_ROTATE real",
"text_attr : common_attr_text",
"text_attr : common_origin",
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
case 43:
#line 195 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_begin(ctx, yyctx->stack.l_mark[-1].un.s); free(yyctx->stack.l_mark[-1].un.s); }
break;
case 44:
#line 197 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_end(ctx); }
break;
case 72:
#line 257 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_begin(ctx); }
break;
case 73:
#line 258 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_end(ctx); }
break;
case 76:
#line 267 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_layer(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 77:
#line 268 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_WIDTH, yyctx->stack.l_mark[0].un.c); }
break;
case 78:
#line 269 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_property(ctx, (pcb_any_obj_t *)ctx->state.poly, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 79:
#line 270 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_add_vertex(ctx, yyctx->stack.l_mark[-2].un.c, yyctx->stack.l_mark[0].un.c); }
break;
case 81:
#line 276 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 82:
#line 277 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_line(ctx); pcb_bxl_reset(ctx); }
break;
case 85:
#line 286 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_layer(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 86:
#line 287 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_ENDP_X, yyctx->stack.l_mark[-2].un.c); pcb_bxl_set_coord(ctx, BXL_ENDP_Y, yyctx->stack.l_mark[0].un.c); }
break;
case 87:
#line 288 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_WIDTH, yyctx->stack.l_mark[0].un.c); }
break;
#line 653 "../../src_plugins/io_bxl/bxl_gram.c"
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
