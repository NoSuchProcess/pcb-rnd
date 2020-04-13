
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
   35,   35,   35,   35,   35,   35,   35,   37,   27,   36,
   36,   38,   38,   38,   38,   38,   40,   28,   39,   39,
   41,   41,   41,   41,   29,   42,   42,   43,   43,   43,
   43,   43,   43,   31,   44,   44,   45,   45,   45,   45,
   45,   45,   32,   46,   46,   47,   47,   47,   47,   47,
   47,   47,   47,   33,   30,   48,   48,   49,   49,   49,
   10,
};
static const pcb_bxl_int_t pcb_bxl_len[] = {                      2,
    2,    0,    3,    1,    1,    1,    1,    1,    1,    1,
    2,    0,    1,    1,    1,    1,    3,    0,    4,    2,
    2,    2,   10,    0,    4,    2,    2,    2,    2,    0,
    2,    4,    0,    4,    2,    2,    2,    2,    0,    6,
    0,    3,    1,    6,    6,    6,    6,    0,    3,    1,
    1,    1,    1,    1,    1,    1,    1,    2,    0,    4,
    2,    2,    2,    2,    4,    2,    2,    0,    3,    0,
    4,    2,    4,    2,    2,    3,    0,    3,    0,    4,
    2,    4,    4,    2,    2,    0,    4,    2,    4,    3,
    2,    2,    2,    2,    0,    4,    2,    4,    2,    2,
    2,    2,    2,    0,    4,    2,    4,    2,    2,    2,
    2,    2,    2,    2,    2,    0,    4,    4,    2,    2,
    6,
};
static const pcb_bxl_int_t pcb_bxl_defred[] = {                   0,
    0,    0,    0,   13,   11,    0,    0,    0,    0,    1,
    0,    4,    5,    6,    7,    0,    0,    0,    0,    0,
    0,   17,    0,    0,   39,    0,    3,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   20,
   21,   22,    0,   26,    8,    9,   27,   28,   29,    0,
    0,    0,    0,    0,    0,    0,    0,   43,    0,   19,
   25,    0,    0,    0,    0,    0,   40,    0,  121,    0,
    0,   14,   15,   16,    0,    0,    0,   42,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   68,   77,    0,
    0,    0,    0,    0,    0,    0,   50,   51,   52,   53,
   54,   55,   56,   57,    0,    0,    0,    0,   23,   31,
    0,   58,    0,    0,    0,   85,    0,   94,    0,  103,
    0,  114,  115,   47,    0,   44,   45,   46,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   69,
    0,   78,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   49,
    0,    0,    0,    0,    0,   32,   61,   62,   63,   64,
    0,   66,   67,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   92,   88,    0,    0,   91,
   93,    0,   99,   97,    0,  100,  101,  102,    0,  111,
  106,    0,  113,  110,  108,  109,  112,    0,    0,  119,
  120,    0,   35,   36,   37,   38,    0,    0,   60,   74,
   72,    0,   75,    0,    0,   84,   81,    0,    0,    0,
    0,   90,   87,    0,   96,    0,  105,    0,  117,   34,
   65,    0,   76,   71,    0,    0,   80,   89,   98,  107,
  118,   73,   82,   83,
};
static const pcb_bxl_int_t pcb_bxl_dgoto[] = {                    2,
   74,   75,    3,   10,   11,    4,   12,   13,   14,   15,
   47,   22,   31,   24,   85,   36,   86,  130,  175,   56,
   38,   57,   58,   95,   96,   97,   98,   99,  100,  101,
  102,  103,  104,  112,  138,  140,  113,  190,  142,  114,
  195,  116,  149,  118,  156,  120,  165,  122,  169,
};
static const pcb_bxl_int_t pcb_bxl_sindex[] = {                  -4,
   -4,    0, -252,    0,    0, -239, -230, -228, -242,    0,
   -4,    0,    0,    0,    0,    2,    5,   -4, -277, -252,
 -191,    0, -233,   -4,    0, -263,    0, -181, -179, -178,
   24, -175, -237, -237, -237,   26, -184, -137, -170,    0,
    0,    0,    2,    0,    0,    0,    0,    0,    0,    5,
   30,   33,   52,   53,   54, -174,   -4,    0, -211,    0,
    0, -152, -146, -201, -201, -201,    0, -137,    0,   -4,
   -4,    0,    0,    0,   70,   71,   73,    0, -151, -183,
 -201, -201, -201, -139, -143, -151,   87,    0,    0,   92,
  104,  108,  109,  109, -131,   -4,    0,    0,    0,    0,
    0,    0,    0,    0,  110,  111,  112,  114,    0,    0,
 -150,    0,  115,  116, -236,    0, -193,    0, -255,    0,
 -274,    0,    0,    0, -183,    0,    0,    0, -176,   -4,
 -101, -102, -100,  -99, -201,  -96, -201,  118, -246,    0,
 -224,    0,  -97,  -93, -201,  -94,  -90, -237,  127, -201,
  -88, -201, -201, -201, -201,  129,  -86,  -85, -201, -201,
  -84,  -83, -237, -237,  134, -201,  -81,  -80,  135,    0,
 -201, -201,  -76,  -73,  145,    0,    0,    0,    0,    0,
  143,    0,    0,   87, -201,  -69, -201,  -70,  147,  153,
 -201,  -62, -201, -201,  155,    0,    0,  154,  -61,    0,
    0,   92,    0,    0,  156,    0,    0,    0,  104,    0,
    0,  157,    0,    0,    0,    0,    0,  108,  158,    0,
    0,  109,    0,    0,    0,    0,  114, -201,    0,    0,
    0,  159,    0, -201,  115,    0,    0,  160,  161,  116,
 -201,    0,    0, -201,    0, -201,    0, -201,    0,    0,
    0, -201,    0,    0, -201, -201,    0,    0,    0,    0,
    0,    0,    0,    0,
};
static const pcb_bxl_int_t pcb_bxl_rindex[] = {                   9,
    1,    0,  197,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  196,  198,    0,    0,  197,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -72,    0,    0,
    0,    0,  196,    0,    0,    0,    0,    0,    0,  198,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -72,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -59,  -71,
    0,    0,    0,    0,    0,  -59,  201,    0,    0,  202,
  203,  204,  205,  205,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  206,    0,    0,
    0,    0,  207,  208,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  -71,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  201,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  202,    0,    0,    0,    0,    0,    0,  203,    0,
    0,    0,    0,    0,    0,    0,    0,  204,    0,    0,
    0,  205,    0,    0,    0,    0,  206,    0,    0,    0,
    0,    0,    0,    0,  207,    0,    0,    0,    0,  208,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,
};
static const pcb_bxl_int_t pcb_bxl_gindex[] = {                   0,
 -108,  -63,    0,  199,    0,   -1,    0,    0,    0,    0,
  -30,  177,    0,  171,  136,    0,    0,   -3,    0,  162,
    0,    0,    0,   98,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   41,    0,   -9,    0,    0,  -13,    0,
    0,   27,    0,   19,    0,   13,    0,  -87,    0,
};
#define pcb_bxl_TABLESIZE 321
static const pcb_bxl_int_t pcb_bxl_table[] = {                    5,
   10,   76,   77,   48,   49,    1,  123,  157,   12,   20,
    6,   72,   73,   19,    7,  166,   25,  105,  106,  107,
   16,  158,   37,   45,   46,    8,  143,  185,  183,   17,
  186,   18,  167,  168,  159,   26,  160,   32,   33,   34,
  144,   21,  161,  187,   23,  207,  208,  162,  163,  191,
   39,  213,  192,  145,  164,   68,   72,   73,  188,    9,
  146,  147,  223,  224,   43,  193,   50,  148,   79,   80,
  194,  181,   28,   29,   30,  189,   40,   35,   41,   42,
  150,  198,   44,  151,   51,   59,  203,   62,  205,  206,
   63,   64,   65,   66,  125,  212,  152,  171,  172,  173,
  174,   87,  219,   69,   67,   70,  153,  154,  155,   88,
   89,   71,   90,   81,   82,   91,   83,  201,   84,   92,
  108,  230,   93,  232,  109,   94,  111,  236,  176,  238,
  239,  115,  216,  217,  249,  131,  132,  133,  134,  135,
  136,  137,   52,  117,   53,   54,   55,  119,  121,  124,
  126,  127,  128,  129,  139,  141,  177,  178,  184,  179,
  180,  182,  196,  197,  251,  199,  200,  202,  204,  209,
  253,  211,  214,  210,  218,  222,  215,  258,  220,  221,
  259,  225,  260,  226,  261,  227,  228,  231,  262,  233,
  234,  263,  264,  235,  237,  240,    2,  241,  242,  244,
  246,  248,  252,  255,  256,   18,   41,   24,   30,   48,
   59,   86,   95,  104,  116,   33,   70,   79,   27,   60,
   61,  110,  170,  250,  229,  254,  257,  245,  243,   78,
  247,    0,    0,    0,    0,    0,    0,    0,    0,    0,
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
    0,   65,   66,   34,   35,   10,   94,  263,    0,   11,
  263,  258,  259,  256,  267,  290,   18,   81,   82,   83,
  260,  277,   24,  261,  262,  278,  263,  274,  137,  260,
  277,  260,  307,  308,  290,  313,  292,  271,  272,  273,
  277,   40,  298,  290,   40,  154,  155,  303,  304,  274,
  314,  160,  277,  290,  310,   57,  258,  259,  305,  312,
  297,  298,  171,  172,   41,  290,   41,  304,   70,   71,
  295,  135,  264,  265,  266,  139,  258,  311,  258,  258,
  274,  145,  258,  277,  269,  256,  150,   58,  152,  153,
   58,   40,   40,   40,   96,  159,  290,  274,  275,  276,
  277,  285,  166,  315,  279,  258,  300,  301,  302,  293,
  294,  258,  296,   44,   44,  299,   44,  148,  270,  303,
  260,  185,  306,  187,  268,  309,   40,  191,  130,  193,
  194,   40,  163,  164,  222,  286,  287,  288,  289,  290,
  291,  292,  280,   40,  282,  283,  284,   40,   40,  281,
   41,   41,   41,   40,   40,   40,  258,  260,   41,  260,
  260,  258,  260,  257,  228,  260,  257,   41,  257,   41,
  234,  257,  257,  260,   41,   41,  260,  241,  260,  260,
  244,  258,  246,  257,  248,   41,   44,  257,  252,  260,
   44,  255,  256,   41,  257,   41,    0,   44,  260,   44,
   44,   44,   44,   44,   44,   10,  279,   10,  268,  281,
   10,   10,   10,   10,   10,   10,   10,   10,   20,   43,
   50,   86,  125,  227,  184,  235,  240,  209,  202,   68,
  218,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
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
#define pcb_bxl_UNDFTOKEN 367
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
0,0,0,0,0,"illegal-symbol",
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
"$$2 :",
"poly : T_POLY $$2 poly_attrs",
"poly_attrs :",
"poly_attrs : '(' poly_attr ')' poly_attrs",
"poly_attr : T_LAYER T_ID",
"poly_attr : T_ORIGIN coord ',' coord",
"poly_attr : T_WIDTH coord",
"poly_attr : T_PROPERTY T_QSTR",
"poly_attr : coord ',' coord",
"$$3 :",
"line : T_LINE $$3 line_attrs",
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
case 39:
#line 185 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_begin(ctx, yyctx->stack.l_mark[-1].un.s); free(yyctx->stack.l_mark[-1].un.s); }
break;
case 40:
#line 187 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_pattern_end(ctx); }
break;
case 68:
#line 247 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_begin(ctx); }
break;
case 69:
#line 248 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_end(ctx); }
break;
case 72:
#line 257 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_layer(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 73:
#line 258 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_ORIGIN_X, yyctx->stack.l_mark[-2].un.c); pcb_bxl_set_coord(ctx, BXL_ORIGIN_Y, yyctx->stack.l_mark[0].un.c); }
break;
case 74:
#line 259 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_WIDTH, yyctx->stack.l_mark[0].un.c); }
break;
case 76:
#line 261 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_poly_add_vertex(ctx, yyctx->stack.l_mark[-2].un.c, yyctx->stack.l_mark[0].un.c); }
break;
case 77:
#line 266 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_reset(ctx); }
break;
case 78:
#line 267 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_add_line(ctx); pcb_bxl_reset(ctx); }
break;
case 81:
#line 276 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_layer(ctx, yyctx->stack.l_mark[0].un.s); free(yyctx->stack.l_mark[0].un.s); }
break;
case 82:
#line 277 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_ORIGIN_X, yyctx->stack.l_mark[-2].un.c); pcb_bxl_set_coord(ctx, BXL_ORIGIN_Y, yyctx->stack.l_mark[0].un.c); }
break;
case 83:
#line 278 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_ENDP_X, yyctx->stack.l_mark[-2].un.c); pcb_bxl_set_coord(ctx, BXL_ENDP_Y, yyctx->stack.l_mark[0].un.c); }
break;
case 84:
#line 279 "../../src_plugins/io_bxl/bxl_gram.y"
	{ pcb_bxl_set_coord(ctx, BXL_WIDTH, yyctx->stack.l_mark[0].un.c); }
break;
#line 657 "../../src_plugins/io_bxl/bxl_gram.c"
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
