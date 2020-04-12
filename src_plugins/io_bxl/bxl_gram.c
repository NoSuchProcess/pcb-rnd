
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
    6,    3,    3,    1,    1,    2,    7,   12,   12,   13,
   13,   13,    8,   14,   14,   16,   16,   16,   16,   15,
   15,   17,   18,   18,   19,   19,   19,   19,   21,    9,
   20,   20,   22,   22,   22,   22,   23,   24,   24,   25,
   25,   25,   25,   25,   25,   25,   25,   26,   34,   34,
   35,   35,   35,   35,   35,   35,   35,   27,   36,   36,
   37,   37,   37,   37,   37,   39,   28,   38,   38,   40,
   40,   40,   40,   29,   41,   41,   42,   42,   42,   42,
   42,   42,   31,   43,   43,   44,   44,   44,   44,   44,
   44,   32,   45,   45,   46,   46,   46,   46,   46,   46,
   46,   46,   33,   30,   47,   47,   48,   48,   48,   10,
};
static const pcb_bxl_int_t pcb_bxl_len[] = {                      2,
    2,    0,    3,    1,    1,    1,    1,    1,    1,    1,
    2,    0,    1,    1,    1,    1,    3,    0,    4,    2,
    2,    2,   10,    0,    4,    2,    2,    2,    2,    0,
    2,    4,    0,    4,    2,    2,    2,    2,    0,    6,
    0,    3,    1,    6,    6,    6,    6,    0,    3,    1,
    1,    1,    1,    1,    1,    1,    1,    2,    0,    4,
    2,    2,    2,    2,    4,    2,    2,    2,    0,    4,
    2,    4,    2,    2,    3,    0,    3,    0,    4,    2,
    4,    4,    2,    2,    0,    4,    2,    4,    3,    2,
    2,    2,    2,    0,    4,    2,    4,    2,    2,    2,
    2,    2,    0,    4,    2,    4,    2,    2,    2,    2,
    2,    2,    2,    2,    0,    4,    4,    2,    2,    6,
};
static const pcb_bxl_int_t pcb_bxl_defred[] = {                   0,
    0,    0,    0,   13,   11,    0,    0,    0,    0,    1,
    0,    4,    5,    6,    7,    0,    0,    0,    0,    0,
    0,   17,    0,    0,   39,    0,    3,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   20,
   21,   22,    0,   26,    8,    9,   27,   28,   29,    0,
    0,    0,    0,    0,    0,    0,    0,   43,    0,   19,
   25,    0,    0,    0,    0,    0,   40,    0,  120,    0,
    0,   14,   15,   16,    0,    0,    0,   42,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   76,    0,
    0,    0,    0,    0,    0,    0,   50,   51,   52,   53,
   54,   55,   56,   57,    0,    0,    0,    0,   23,   31,
    0,   58,    0,   68,    0,    0,   84,    0,   93,    0,
  102,    0,  113,  114,   47,    0,   44,   45,   46,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   77,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   49,    0,    0,    0,    0,    0,
   32,   61,   62,   63,   64,    0,   66,   67,    0,   73,
   71,    0,   74,    0,    0,    0,    0,    0,    0,    0,
   91,   87,    0,    0,   90,   92,    0,   98,   96,    0,
   99,  100,  101,    0,  110,  105,    0,  112,  109,  107,
  108,  111,    0,    0,  118,  119,    0,   35,   36,   37,
   38,    0,    0,   60,    0,   75,   70,   83,   80,    0,
    0,    0,    0,   89,   86,    0,   95,    0,  104,    0,
  116,   34,   65,   72,    0,    0,   79,   88,   97,  106,
  117,   81,   82,
};
static const pcb_bxl_int_t pcb_bxl_dgoto[] = {                    2,
   74,   75,    3,   10,   11,    4,   12,   13,   14,   15,
   47,   22,   31,   24,   85,   36,   86,  131,  180,   56,
   38,   57,   58,   95,   96,   97,   98,   99,  100,  101,
  102,  103,  104,  112,  139,  114,  145,  147,  115,  200,
  117,  154,  119,  161,  121,  170,  123,  174,
};
static const pcb_bxl_int_t pcb_bxl_sindex[] = {                  -4,
   -4,    0, -255,    0,    0, -250, -227, -218, -217,    0,
   -4,    0,    0,    0,    0,    6,    8,   -4, -259, -255,
 -180,    0, -241,   -4,    0, -248,    0, -194, -187, -182,
   37, -176, -236, -236, -236,   50, -172, -267, -163,    0,
    0,    0,    6,    0,    0,    0,    0,    0,    0,    8,
   44,   45,   69,   71,   72, -166,   -4,    0, -200,    0,
    0, -142, -140, -169, -169, -169,    0, -267,    0,   -4,
   -4,    0,    0,    0,   75,   77,   78,    0, -138, -189,
 -169, -169, -169, -126, -131, -138,  106,  107,    0,  108,
  109,  110,  111,  111, -129,   -4,    0,    0,    0,    0,
    0,    0,    0,    0,  112,  113,  114,  116,    0,    0,
 -162,    0, -222,    0,  117, -239,    0, -202,    0, -263,
    0, -245,    0,    0,    0, -189,    0,    0,    0, -132,
   -4, -100, -101,  -99,  -98, -169,  -95, -169,  119, -169,
  -93, -169,  -94,  121,  126, -221,    0,  -92,  -88, -169,
  -89,  -84, -236,  133, -169,  -82, -169, -169, -169, -169,
  135,  -83,  -79, -169, -169,  -78,  -76, -236, -236,  140,
 -169,  -74,  -72,  141,    0, -169, -169,  -69,  -67,  150,
    0,    0,    0,    0,    0,  151,    0,    0,  106,    0,
    0,  152,    0, -169,  107, -169,  -60, -169, -169,  153,
    0,    0,  154,  -61,    0,    0,  108,    0,    0,  156,
    0,    0,    0,  109,    0,    0,  157,    0,    0,    0,
    0,    0,  110,  158,    0,    0,  111,    0,    0,    0,
    0,  116, -169,    0, -169,    0,    0,    0,    0,  159,
  160,  117, -169,    0,    0, -169,    0, -169,    0, -169,
    0,    0,    0,    0, -169, -169,    0,    0,    0,    0,
    0,    0,    0,
};
static const pcb_bxl_int_t pcb_bxl_rindex[] = {                   9,
    1,    0,  205,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  196,  197,    0,    0,  205,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -71,    0,    0,
    0,    0,  196,    0,    0,    0,    0,    0,    0,  197,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -71,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -59,  -70,
    0,    0,    0,    0,    0,  -59,  200,  202,    0,  203,
  204,  206,  207,  207,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  208,    0,    0,
    0,    0,    0,    0,  209,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  -70,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  200,    0,
    0,    0,    0,    0,  202,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  203,    0,    0,    0,
    0,    0,    0,  204,    0,    0,    0,    0,    0,    0,
    0,    0,  206,    0,    0,    0,  207,    0,    0,    0,
    0,  208,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  209,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
static const pcb_bxl_int_t pcb_bxl_gindex[] = {                   0,
 -116,  -63,    0,  195,    0,   10,    0,    0,    0,    0,
  -30,  177,    0,  171,  136,    0,    0,   -9,    0,  161,
    0,    0,    0,   98,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   36,    0,   31,    0,  -15,    0,    0,
   21,    0,   16,    0,   11,    0,  -87,    0,
};
#define pcb_bxl_TABLESIZE 321
static const pcb_bxl_int_t pcb_bxl_table[] = {                  162,
   10,   76,   77,   48,   49,    1,  124,    6,   12,   16,
    5,    7,   52,  163,   53,   54,   55,  105,  106,  107,
   20,  188,    8,  148,   45,   46,  164,   25,  165,   32,
   33,   34,   17,   37,  166,   72,   73,  149,   19,  167,
  168,   18,  212,  213,  171,   21,  169,   23,  218,  144,
  150,  140,  196,   26,  141,  197,    9,  151,  152,  228,
  229,  172,  173,   40,  153,   39,   68,  142,  198,   35,
   41,  155,  186,  199,  156,   42,  190,   43,  192,   79,
   80,   44,  143,   28,   29,   30,  203,  157,   72,   73,
   50,  208,   59,  210,  211,   87,   51,  158,  159,  160,
  217,   62,   63,   88,   89,  126,   90,  224,   64,   91,
   65,   66,   67,   92,   69,   70,   93,   71,   81,   94,
   82,   83,  206,  132,  133,  134,  135,  136,  137,  138,
  236,   84,  238,  108,  240,  241,  109,  221,  222,  251,
  181,  176,  177,  178,  179,  111,  113,  116,  118,  120,
  122,  125,  127,  128,  129,  130,  146,  182,  183,  189,
  184,  185,  187,  191,  194,  193,  195,  201,  202,  253,
  204,  254,  205,  207,  209,  214,  215,  216,  219,  258,
  223,  227,  259,  220,  260,  225,  261,  226,  230,  231,
  232,  262,  263,  242,  233,  235,  239,  243,  244,  246,
  248,  250,  255,  256,    2,   18,   24,   41,   30,   59,
   48,   69,   85,   94,   27,  103,  115,   33,   78,   60,
   61,  110,  252,  175,  234,  237,  257,  245,   78,  247,
    0,    0,    0,  249,    0,    0,    0,    0,    0,    0,
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
static const pcb_bxl_int_t pcb_bxl_check[] = {                  263,
    0,   65,   66,   34,   35,   10,   94,  263,    0,  260,
    1,  267,  280,  277,  282,  283,  284,   81,   82,   83,
   11,  138,  278,  263,  261,  262,  290,   18,  292,  271,
  272,  273,  260,   24,  298,  258,  259,  277,  256,  303,
  304,  260,  159,  160,  290,   40,  310,   40,  165,  113,
  290,  274,  274,  313,  277,  277,  312,  297,  298,  176,
  177,  307,  308,  258,  304,  314,   57,  290,  290,  311,
  258,  274,  136,  295,  277,  258,  140,   41,  142,   70,
   71,  258,  305,  264,  265,  266,  150,  290,  258,  259,
   41,  155,  256,  157,  158,  285,  269,  300,  301,  302,
  164,   58,   58,  293,  294,   96,  296,  171,   40,  299,
   40,   40,  279,  303,  315,  258,  306,  258,   44,  309,
   44,   44,  153,  286,  287,  288,  289,  290,  291,  292,
  194,  270,  196,  260,  198,  199,  268,  168,  169,  227,
  131,  274,  275,  276,  277,   40,   40,   40,   40,   40,
   40,  281,   41,   41,   41,   40,   40,  258,  260,   41,
  260,  260,  258,  257,   44,  260,   41,  260,  257,  233,
  260,  235,  257,   41,  257,   41,  260,  257,  257,  243,
   41,   41,  246,  260,  248,  260,  250,  260,  258,  257,
   41,  255,  256,   41,   44,   44,  257,   44,  260,   44,
   44,   44,   44,   44,    0,   10,   10,  279,  268,   10,
  281,   10,   10,   10,   20,   10,   10,   10,   10,   43,
   50,   86,  232,  126,  189,  195,  242,  207,   68,  214,
   -1,   -1,   -1,  223,   -1,   -1,   -1,   -1,   -1,   -1,
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
#define pcb_bxl_UNDFTOKEN 366
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
0,0,0,0,"illegal-symbol",
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
"$$2 :",
"line : T_LINE $$2 line_attrs",
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
case 16:
#line 120 "../../src_plugins/io_bxl/bxl_gram.y"
	{ yyctx->val.un.c = PCB_MIL_TO_COORD(yyctx->stack.l_mark[0].un.d); }
break;
case 39:
#line 185 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_begin(ctx, yyctx->stack.l_mark[-1].un.s); free(yyctx->stack.l_mark[-1].un.s); }
break;
case 40:
#line 187 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_end(ctx); }
break;
case 76:
#line 265 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 77:
#line 266 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_line(ctx); pcb_bxl_reset(ctx); }
break;
case 80:
#line 275 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_layer(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 81:
#line 276 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_ORIGIN_X, yyctx->stack.l_mark[-2].un.c); pcb_bxl_set_coord(ctx, BXL_ORIGIN_Y, yyctx->stack.l_mark[0].un.c); }
break;
case 82:
#line 277 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_ENDP_X, yyctx->stack.l_mark[-2].un.c); pcb_bxl_set_coord(ctx, BXL_ENDP_Y, yyctx->stack.l_mark[0].un.c); }
break;
case 83:
#line 278 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_WIDTH, yyctx->stack.l_mark[0].un.c); }
break;
#line 622 "../../src_plugins/io_bxl/bxl_gram.c"
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
