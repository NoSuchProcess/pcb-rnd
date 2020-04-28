static const unsigned char pcb_bxl_nfa_0[] = {4,3,0,0,0,0,0,32,255,131,0,0,0,40,0,0,0,0,11,3,0,0,0,0,0,32,255,131,0,0,0,40,0,0,0,0,0,0,0};
static const unsigned char pcb_bxl_bittab_0[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char pcb_bxl_chrtyp_0[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
static const unsigned char pcb_bxl_nfa_1[] = {4,11,3,0,0,0,0,0,32,255,131,0,0,0,40,0,0,0,0,0,3,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,11,3,0,0,0,0,0,40,255,3,32,0,0,0,32,0,0,0,0,0,0,0};
static const unsigned char pcb_bxl_nfa_2[] = {4,3,0,0,0,0,0,0,0,0,254,255,255,7,254,255,255,7,11,3,0,0,0,0,0,32,255,3,254,255,255,135,254,255,255,7,0,0,0};
static const unsigned char pcb_bxl_nfa_3[] = {4,1,34,6,1,11,3,255,255,255,255,251,255,255,255,255,255,255,255,255,255,255,255,0,7,1,1,34,0,0};
static const unsigned char pcb_bxl_nfa_4[] = {4,3,0,4,0,0,0,19,0,4,0,0,0,0,0,0,0,0,0};
static const unsigned char pcb_bxl_nfa_5[] = {4,3,0,34,0,0,1,0,0,0,0,0,0,0,0,0,0,0,11,3,0,34,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
/* strtree.h BEGIN { */
#ifndef UREGLEX_STRTREE_H
#define UREGLEX_STRTREE_H 
typedef enum {ULX_REQ = 1, ULX_BRA, ULX_FIN, ULX_BAD} ureglex_stree_op_t;
typedef struct ureglex_strtree_s { int *code, *ip; } ureglex_strtree_t;
int ureglex_strtree_exec(ureglex_strtree_t *ctx, int chr);
#define UREGLEX_STRTREE_MORE -5
#endif
/* strtree.h END } */

/* strtree_exec.c BEGIN { */
int ureglex_strtree_exec(ureglex_strtree_t *ctx, int chr)
{
 int expected, dst;
 for(;;) {
  switch(*ctx->ip) {
   case ULX_REQ:
    expected = ctx->ip[1];
    ctx->ip += 2;
    if (chr == expected)
     return UREGLEX_STRTREE_MORE;
    return -1;
   case ULX_BRA:
    expected = ctx->ip[1];
    dst = ctx->ip[2];
    ctx->ip += 3;
    if (chr == expected) {
     ctx->ip = ctx->code + dst;
     return UREGLEX_STRTREE_MORE;
    }
    break;
   case ULX_FIN: return ctx->ip[1];
   case ULX_BAD: return -2;
  }
 }
 return -1;
}
/* strtree_exec.c END } */

/* exec.h BEGIN { */
#ifndef UREGLEX_EXEC_COMMON_H
#define UREGLEX_EXEC_COMMON_H 
#define MAXTAG 10
typedef struct ureglex_precomp_s {
 const unsigned char *nfa;
 const unsigned char *bittab;
 const unsigned char *chrtyp;
 double weight;
} ureglex_precomp_t;
typedef struct ureglex_s {
 ureglex_precomp_t *pc;
 const char *bol;
 const char *bopat[MAXTAG];
 const char *eopat[MAXTAG];
 int score;
 const char *endp;
 union { const void *ptr; int i; } pmstk[30];
 int pmsp;
 const unsigned char *pm_ap;
 const char *pm_lp;
 int pm_c;
 const char *pm_bp;
 const char *pm_ep;
 const char *pm_are;
 const char *ex_lp;
 unsigned char ex_c;
 int ex_loop, pm_loop, pm_loop2, pm_loop2_later;
 int exec_state;
} ureglex_t;
typedef enum {
 UREGLEX_MORE = -1,
 UREGLEX_TOO_LONG = -2,
 UREGLEX_NO_MATCH = -3,
 UREGLEX_NOP = -4
} ureglex_error_t;
extern const unsigned char ureglex_nfa_str[];
#define ULX_BUF ctx->buff
#define ULX_TAGP(n) (ctx->state[ruleid].bopat[(n)])
#define ULX_TAGL(n) (ctx->state[ruleid].eopat[(n)] - ctx->state[ruleid].bopat[(n)])
#define ULX_IGNORE goto ureglex_ignore;
#endif
/* exec.h END } */

/* common.h, exec.c BEGIN { */
#define MAXCHR 128
#define CHRBIT 8
#define BITBLK MAXCHR/CHRBIT
#define BLKIND 0170
#define BITIND 07
void ureglex_exec_init(ureglex_t *re, const char *str, int buff_used);
extern int ureglex_exec(ureglex_t *re);
extern int ureglex_tag(ureglex_t *re, int tagid, char **begin, char **end);
enum ureglex_opcode_e {
 NOP = 0,
 END = 0,
 CHR = 1,
 ANY = 2,
 CCL = 3,
 BOL = 4,
 EOL = 5,
 BOT = 6,
 EOT = 7,
 BOW = 8,
 EOW = 9,
 REF = 10,
 CLO = 11
};
#include <string.h>
#include <assert.h>
#define re_tolower(c) c
#define iswordc(r,x) r->pc->chrtyp[inascii(x)]
#define end(s) (*(s) == '\0')
static unsigned char ureglex_bitarr[] = {1,2,4,8,16,32,64,128};
#define inascii(x) (0177&(x))
#define isinset(x,y) ((x)[((y)&BLKIND)>>3] & ureglex_bitarr[(y)&BITIND])
const unsigned char ureglex_nfa_str[] = {0};
#define ANYSKIP 2
#define CHRSKIP 3
#define CCLSKIP 18
static const char MORE[] = "more!";
#define want_more(loopid) \
do { \
 if (r->pm_lp < r->endp) \
  goto loop ## loopid; \
 r->pm_loop = loopid; \
 return MORE; \
} while(0)
#define PUSH(r,ty,val) if (r->pmsp >= sizeof(r->pmstk) / sizeof(r->pmstk[0])) return 0; r->pmstk[r->pmsp++].ty = val
#define POP(r,ty,dst) dst = r->pmstk[--r->pmsp].ty
static const char *pmatch(ureglex_t *r)
{
 register int op, c, n;
 register const char *e = NULL;
 switch(r->pm_loop) {
  case 1: r->pm_loop = 0; goto loop1;
  case 2: r->pm_loop = 0; goto loop2;
  case 3: r->pm_loop = 0; goto loop3;
  case 4: r->pm_loop = 0; goto loop4;
  case 6: r->pm_loop = 0; goto loop6;
  case 7: r->pm_loop = 0; goto loop7;
  case 8: r->pm_loop = 0; goto loop8;
 }
 switch(r->pm_loop2) {
  case 1: r->pm_loop2 = 0; goto loop2_1;
 }
 while ((op = *r->pm_ap++) != END)
  switch(op) {
  case CHR:
   want_more(6);
   loop6:;
   if (re_tolower(*r->pm_lp++) != *r->pm_ap++)
    return 0;
   r->score += 100;
   break;
  case ANY:
   want_more(7);
   loop7:;
   if (end(r->pm_lp++))
    return 0;
   r->score++;
   break;
  case CCL:
   if (end(r->pm_lp))
    return 0;
   want_more(8);
   loop8:;
   c = re_tolower(*r->pm_lp++);
   if (!isinset(r->pm_ap,c))
    return 0;
   r->pm_ap += BITBLK;
   r->score += 2;
   break;
  case BOL:
   if (r->pm_lp != r->bol)
    return 0;
   r->score += 10;
   break;
  case EOL:
   if (!end(r->pm_lp))
    return 0;
   r->score += 10;
   break;
  case BOT:
   r->bopat[*r->pm_ap++] = r->pm_lp;
   break;
  case EOT:
   r->eopat[*r->pm_ap++] = r->pm_lp;
   break;
  case BOW:
   if ((r->pm_lp!=r->bol && iswordc(r, r->pm_lp[-1])) || !iswordc(r, *r->pm_lp))
    return 0;
   r->score += 5;
   break;
  case EOW:
   if (r->pm_lp==r->bol || !iswordc(r, r->pm_lp[-1]) || (!end(r->pm_lp) && iswordc(r, *r->pm_lp)))
    return 0;
   r->score += 5;
   break;
  case REF:
   n = *r->pm_ap++;
   r->pm_bp = r->bopat[n];
   r->pm_ep = r->eopat[n];
   while (r->pm_bp < r->pm_ep) {
    want_more(1);
    loop1:;
    if (*r->pm_bp++ != *r->pm_lp++)
     return 0;
    r->score += 2;
   }
   break;
  case CLO:
   r->pm_are = r->pm_lp;
   switch(*r->pm_ap) {
   case ANY:
    do {
     want_more(2);
     loop2:;
    } while(!end(r->pm_lp++));
    n = ANYSKIP;
    r->score++;
    break;
   case CHR:
    r->pm_c = *(r->pm_ap+1);
    do {
     want_more(3);
     loop3:;
    } while (!end(r->pm_lp) && r->pm_c == re_tolower(*r->pm_lp) && (r->pm_lp++));
    n = CHRSKIP;
    r->score += 100;
    break;
   case CCL:
    do {
     want_more(4);
     loop4:;
    } while ((c = re_tolower(*r->pm_lp)) && isinset(r->pm_ap+1,c) && (r->pm_lp++));
    n = CCLSKIP;
    r->score += 2;
    break;
   default:
    return 0;
   }
   r->pm_ap += n;
   while (r->pm_lp >= r->pm_are) {
    PUSH(r, ptr, r->pm_ap);
    PUSH(r, ptr, r->pm_lp);
    PUSH(r, i, r->pm_loop);
    r->pm_loop2_later = 1;
    e = pmatch(r);
    if (e == MORE)
     return MORE;
    loop2_1:;
    POP(r, i, r->pm_loop);
    POP(r, ptr, r->pm_lp);
    POP(r, ptr, r->pm_ap);
    if (e)
     return e;
    --r->pm_lp;
   }
   return 0;
  default:
   return 0;
  }
 r->pm_loop2 = r->pm_loop2_later;
 return r->pm_lp;
}
void ureglex_exec_init(ureglex_t *r, const char *lp, int buff_used)
{
 r->bol = lp;
 r->score = 1;
 memset(r->bopat, 0, (char *)&r->eopat[MAXTAG] - (char *)&r->bopat[0]);
 r->pmsp = 0;
 r->ex_lp = lp;
 r->endp = lp + buff_used;
 r->ex_loop = r->pm_loop = r->pm_loop2 = 0;
 r->exec_state = -1;
}
#undef want_more
#define want_more(loopid) \
do { \
 if (r->ex_lp < r->endp) \
  goto loop ## loopid; \
 r->ex_loop = loopid; \
 return -1; \
} while(0)
#define want_more2(loopid) \
do { \
 if (r->pm_lp < r->endp) \
  goto loop ## loopid; \
 r->ex_loop = loopid; \
 return -1; \
} while(0)
int ureglex_exec(ureglex_t *r)
{
 register const char *ep = 0;
 const unsigned char *ap = r->pc->nfa;
 r->endp++;
 switch(r->ex_loop) {
  case 1: r->ex_loop = 0; goto loop1;
  case 2: r->ex_loop = 0; goto loop2;
  case 3: r->ex_loop = 0; goto loop3;
  case 4: r->ex_loop = 0; goto loop4;
 }
 switch(*ap) {
 case BOL:
  r->pm_ap = ap;
  r->pm_lp = r->ex_lp;
  loop1:;
  ep = pmatch(r);
  if (ep == MORE)
   want_more2(1);
  break;
 case CHR:
  r->ex_c = *(ap+1);
  while (!end(r->ex_lp) && re_tolower(*r->ex_lp) != r->ex_c) {
   r->ex_lp++;
   want_more(2);
   loop2:;
  }
  if (end(r->ex_lp))
   return 0;
 default:
  for(;;) {
   r->pm_ap = ap;
   r->pm_lp = r->ex_lp;
   loop3:;
   ep = pmatch(r);
   if (ep == MORE) {
    want_more2(3);
   }
   if (ep != NULL)
    break;
   r->ex_lp++;
   want_more(4);
   loop4:;
   if (end(r->ex_lp))
    break;
  }
  break;
 case END:
  return 0;
 }
 if (!ep)
  return 0;
 r->bopat[0] = r->ex_lp;
 r->eopat[0] = ep;
 return r->score;
}
#define setout(dest,val) \
 if ((dest) != NULL) \
  *(dest) = val;
int ureglex_tag(ureglex_t *re, int tagid, char **begin, char **end)
{
 if ((tagid < 0) || (tagid > MAXTAG)) {
  setout(begin, NULL);
  setout(end, NULL);
  return -1;
 }
 setout(begin, (char *)re->bopat[tagid]);
 setout(end, (char *)re->eopat[tagid]);
 return 0;
}
/* common.h, exec.c END } */

int pcb_bxl_strings[] = {2,51,266,2,65,169,2,67,75,2,68,243,2,69,58,2,70,259,2,71,224,2,72,252,2,73,153,2,74,176,2,76,191,2,78,217,2,79,198,2,80,137,2,82,162,2,83,94,2,84,121,2,86,110,2,87,128,4,1,110,1,100,2,67,305,2,68,344,2,80,337,2,83,324,4,1,111,1,109,1,112,1,111,1,110,1,101,1,110,1,116,3,61,4,2,104,832,2,116,800,2,117,819,2,119,781,2,121,770,4,1,97,1,114,2,68,897,2,78,906,4,2,101,843,2,114,850,4,1,105,2,100,924,2,122,915,4,2,97,631,2,105,638,2,108,645,2,111,624,2,114,609,4,1,115,2,70,485,2,86
,500,4,2,97,748,2,111,759,4,2,114,279,2,116,284,4,1,117,1,115,1,116,1,105,1,102,1,121,3,46,4,2,97,522,2,105,515,4,1,114,1,105,1,103,1,105,1,110,2,80,566,2,97,555,3,37,4,2,111,542,2,117,531,4,1,108,1,117,1,101,1,80,1,111,1,105,1,110,1,116,3,31,4,1,97,1,116,1,97,3,27,4,2,101,459,2,111,470,4,2,97,407,2,111,393,4,1,68,1,95,1,68,1,88,1,70,3,6,4,1,99,3,47,4,1,116,1,114,2,105,294,3,45,4,1,98,1,117,1,116,1,101,3,44,4,1,111,1,109,1,112,1,111,1,110,1,101,1,110,1,116,3,62,4,1,121,1,109,1,98,1,111,1,108,3,60
,4,2,97,362,2,111,353,4,1,97,1,116,1,97,3,28,4,1,105,1,110,1,116,3,42,4,2,100,380,2,116,369,4,1,116,1,101,1,114,1,110,3,26,4,1,83,1,116,1,97,1,99,1,107,3,14,4,1,110,1,116,2,67,429,2,72,416,2,87,448,4,1,108,1,115,1,101,3,8,4,1,101,1,105,1,103,1,104,1,116,3,12,4,1,104,1,97,1,114,1,87,1,105,1,100,1,116,1,104,3,11,4,1,105,1,100,1,116,1,104,3,10,4,1,105,1,103,1,104,1,116,3,22,4,1,108,1,101,1,68,1,105,1,97,1,109,3,17,4,1,108,1,105,1,112,1,112,1,101,1,100,3,53,4,1,105,1,115,1,105,1,98,1,108,1,101,3,52,4
,1,110,1,101,3,41,4,1,121,1,101,1,114,3,24,4,1,109,1,98,1,101,1,114,3,33,4,1,80,1,97,1,115,1,116,1,101,3,20,4,1,108,1,80,2,97,594,2,105,577,4,1,111,1,105,1,110,1,116,3,29,4,1,110,1,78,1,117,1,109,1,98,1,101,1,114,3,38,4,1,100,1,83,1,116,1,121,1,108,1,101,3,36,4,1,111,1,112,1,101,1,114,1,116,1,121,3,54,4,1,108,1,121,3,40,4,2,100,656,2,116,665,4,2,99,733,2,110,722,4,1,97,1,116,1,101,1,100,3,19,4,2,83,676,2,84,683,3,32,4,1,116,1,101,1,114,1,110,3,25,4,2,104,699,2,116,692,4,1,121,1,112,1,101,3,23,4,2
,97,715,2,121,708,4,1,97,1,112,1,101,3,16,4,1,108,1,101,3,35,4,1,99,1,107,3,13,4,1,78,1,97,1,109,1,101,3,34,4,1,107,1,80,1,111,1,105,1,110,1,116,3,30,4,1,100,1,105,1,117,1,115,3,48,4,1,116,1,97,1,116,1,101,3,39,4,1,109,1,98,1,111,1,108,3,59,4,1,101,1,101,1,112,1,65,1,110,1,103,1,108,1,101,3,50,4,1,97,1,114,1,116,1,65,1,110,1,103,1,108,1,101,3,49,4,1,114,1,102,1,97,1,99,1,101,3,18,4,1,97,1,112,1,101,1,115,3,15,4,2,109,857,2,120,878,4,1,117,1,101,3,7,4,1,112,1,108,1,97,1,116,1,101,1,100,1,97,1,116,1
,97,3,56,4,1,116,2,83,886,3,51,4,1,116,1,121,1,108,1,101,3,9,4,1,97,1,116,1,97,3,58,4,1,97,1,109,1,101,3,57,4,1,97,1,114,1,100,3,55,4,1,116,1,104,3,21,4};
ureglex_precomp_t pcb_bxl_rules[] = {
	{pcb_bxl_nfa_0, pcb_bxl_bittab_0, pcb_bxl_chrtyp_0, 1.000000},
	{pcb_bxl_nfa_1, pcb_bxl_bittab_0, pcb_bxl_chrtyp_0, 1.000000},
	{pcb_bxl_nfa_2, pcb_bxl_bittab_0, pcb_bxl_chrtyp_0, 1.000000},
	{pcb_bxl_nfa_3, pcb_bxl_bittab_0, pcb_bxl_chrtyp_0, 1.000000},
	{pcb_bxl_nfa_4, pcb_bxl_bittab_0, pcb_bxl_chrtyp_0, 1.000000},
	{pcb_bxl_nfa_5, pcb_bxl_bittab_0, pcb_bxl_chrtyp_0, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{NULL, NULL, NULL, 0.0}
};
#define pcb_bxl_num_rules 63
typedef struct pcb_bxl_ureglex_s {
	ureglex_precomp_t *rules;
	char buff[256];
	int num_rules, buff_used, step_back_to, buff_save_term, by_len;
	long loc_offs[2], loc_line[2], loc_col[2];
	ureglex_t state[pcb_bxl_num_rules];
	const char *sp;
	int strtree_state, strtree_len, strtree_score;
	ureglex_strtree_t strtree;
} pcb_bxl_ureglex_t;

/* TOP CODE BEGIN { */
#line  5 "bxl_lex.ul"
	/*
	 *                            COPYRIGHT
	 *
	 *  pcb-rnd, interactive printed circuit board design
	 *
	 *  BXL IO plugin - bxl lexer
	 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
	 *  (Supported by NLnet NGI0 PET Fund in 2020)
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
	 */
	#include <stdlib.h>
	#include <librnd/core/compat_misc.h>
	#include "bxl_gram.h"
	#define lval  ((pcb_bxl_STYPE *)(user_ctx))
/* TOP CODE END } */

/* exec_spec.h BEGIN { */
void pcb_bxl_lex_reset(pcb_bxl_ureglex_t *ctx);
void pcb_bxl_lex_init(pcb_bxl_ureglex_t *ctx, ureglex_precomp_t *rules);
int pcb_bxl_lex_char(pcb_bxl_ureglex_t *ctx, void *user_ctx, int chr);
/* exec_spec.h END } */

static int pcb_bxl_user_code(pcb_bxl_ureglex_t *ctx, void * user_ctx, int ruleid) {
	ureglex_t *rule = &ctx->state[ruleid];
	(void)rule;
	switch(ruleid) {
		case 0:{
#line  41 "bxl_lex.ul"
	lval->un.i = atoi(ULX_BUF);
	return T_INTEGER;

		}
		return UREGLEX_NOP;
		case 1:{
#line  47 "bxl_lex.ul"
	lval->un.d = strtod(ULX_BUF, NULL);
	return T_REAL_ONLY;

		}
		return UREGLEX_NOP;
		case 2:{
#line  53 "bxl_lex.ul"
	lval->un.s = rnd_strdup(ULX_BUF);
	return T_ID;

		}
		return UREGLEX_NOP;
		case 3:{
#line  59 "bxl_lex.ul"
	lval->un.s = rnd_strndup(ULX_TAGP(1), ULX_TAGL(1));
	return T_QSTR;

		}
		return UREGLEX_NOP;
		case 4:{
#line  65 "bxl_lex.ul"
	return *ULX_BUF;

		}
		return UREGLEX_NOP;
		case 5:{
#line  70 "bxl_lex.ul"
	ULX_IGNORE;

		}
		return UREGLEX_NOP;
		case 6:{
           lval->un.s = rnd_strdup(ULX_BUF); return T_ID;
		}
		return UREGLEX_NOP;
		case 7:{
             return T_TRUE;
		}
		return UREGLEX_NOP;
		case 8:{
            return T_FALSE;
		}
		return UREGLEX_NOP;
		case 9:{
        return T_TEXTSTYLE;
		}
		return UREGLEX_NOP;
		case 10:{
        return T_FONTWIDTH;
		}
		return UREGLEX_NOP;
		case 11:{
    return T_FONTCHARWIDTH;
		}
		return UREGLEX_NOP;
		case 12:{
       return T_FONTHEIGHT;
		}
		return UREGLEX_NOP;
		case 13:{
         return T_PADSTACK;
		}
		return UREGLEX_NOP;
		case 14:{
      return T_ENDPADSTACK;
		}
		return UREGLEX_NOP;
		case 15:{
           return T_SHAPES;
		}
		return UREGLEX_NOP;
		case 16:{
         return T_PADSHAPE;
		}
		return UREGLEX_NOP;
		case 17:{
         return T_HOLEDIAM;
		}
		return UREGLEX_NOP;
		case 18:{
          return T_SURFACE;
		}
		return UREGLEX_NOP;
		case 19:{
           return T_PLATED;
		}
		return UREGLEX_NOP;
		case 20:{
          return T_NOPASTE;
		}
		return UREGLEX_NOP;
		case 21:{
            return T_WIDTH;
		}
		return UREGLEX_NOP;
		case 22:{
           return T_HEIGHT;
		}
		return UREGLEX_NOP;
		case 23:{
          return T_PADTYPE;
		}
		return UREGLEX_NOP;
		case 24:{
            return T_LAYER;
		}
		return UREGLEX_NOP;
		case 25:{
          return T_PATTERN;
		}
		return UREGLEX_NOP;
		case 26:{
       return T_ENDPATTERN;
		}
		return UREGLEX_NOP;
		case 27:{
             return T_DATA;
		}
		return UREGLEX_NOP;
		case 28:{
          return T_ENDDATA;
		}
		return UREGLEX_NOP;
		case 29:{
      return T_ORIGINPOINT;
		}
		return UREGLEX_NOP;
		case 30:{
        return T_PICKPOINT;
		}
		return UREGLEX_NOP;
		case 31:{
        return T_GLUEPOINT;
		}
		return UREGLEX_NOP;
		case 32:{
              return T_PAD;
		}
		return UREGLEX_NOP;
		case 33:{
           return T_NUMBER;
		}
		return UREGLEX_NOP;
		case 34:{
          return T_PINNAME;
		}
		return UREGLEX_NOP;
		case 35:{
         return T_PADSTYLE;
		}
		return UREGLEX_NOP;
		case 36:{
 return T_ORIGINALPADSTYLE;
		}
		return UREGLEX_NOP;
		case 37:{
           return T_ORIGIN;
		}
		return UREGLEX_NOP;
		case 38:{
return T_ORIGINALPINNUMBER;
		}
		return UREGLEX_NOP;
		case 39:{
           return T_ROTATE;
		}
		return UREGLEX_NOP;
		case 40:{
             return T_POLY;
		}
		return UREGLEX_NOP;
		case 41:{
             return T_LINE;
		}
		return UREGLEX_NOP;
		case 42:{
         return T_ENDPOINT;
		}
		return UREGLEX_NOP;
		case 43:{
            return T_WIDTH;
		}
		return UREGLEX_NOP;
		case 44:{
        return T_ATTRIBUTE;
		}
		return UREGLEX_NOP;
		case 45:{
             return T_ATTR;
		}
		return UREGLEX_NOP;
		case 46:{
          return T_JUSTIFY;
		}
		return UREGLEX_NOP;
		case 47:{
              return T_ARC;
		}
		return UREGLEX_NOP;
		case 48:{
           return T_RADIUS;
		}
		return UREGLEX_NOP;
		case 49:{
       return T_STARTANGLE;
		}
		return UREGLEX_NOP;
		case 50:{
       return T_SWEEPANGLE;
		}
		return UREGLEX_NOP;
		case 51:{
             return T_TEXT;
		}
		return UREGLEX_NOP;
		case 52:{
        return T_ISVISIBLE;
		}
		return UREGLEX_NOP;
		case 53:{
        return T_ISFLIPPED;
		}
		return UREGLEX_NOP;
		case 54:{
         return T_PROPERTY;
		}
		return UREGLEX_NOP;
		case 55:{
           return T_WIZARD;
		}
		return UREGLEX_NOP;
		case 56:{
     return T_TEMPLATEDATA;
		}
		return UREGLEX_NOP;
		case 57:{
          return T_VARNAME;
		}
		return UREGLEX_NOP;
		case 58:{
          return T_VARDATA;
		}
		return UREGLEX_NOP;
		case 59:{
           return T_SYMBOL;
		}
		return UREGLEX_NOP;
		case 60:{
        return T_ENDSYMBOL;
		}
		return UREGLEX_NOP;
		case 61:{
        return T_COMPONENT;
		}
		return UREGLEX_NOP;
		case 62:{
     return T_ENDCOMPONENT;
		}
		return UREGLEX_NOP;
	}
	return UREGLEX_NO_MATCH;
	goto ureglex_ignore;
	ureglex_ignore:;
	pcb_bxl_lex_reset(ctx);
	return UREGLEX_MORE;
}

/* exec_spec.c BEGIN { */
void pcb_bxl_lex_reset(pcb_bxl_ureglex_t *ctx)
{
 int n = 0;
 if ((ctx->step_back_to >= 0) && (ctx->step_back_to < ctx->buff_used)) {
  if (ctx->buff_save_term > 0)
   ctx->buff[ctx->step_back_to] = ctx->buff_save_term;
  n = ctx->buff_used - ctx->step_back_to;
  memmove(ctx->buff, ctx->buff + ctx->step_back_to, n+1);
 }
 ctx->buff_used = n;
 for(n = 0; n < ctx->num_rules; n++)
  ureglex_exec_init(&ctx->state[n], ctx->buff, ctx->buff_used);
 ctx->buff_save_term = ctx->step_back_to = -1;
 ctx->loc_offs[0] = ctx->loc_offs[1];
 ctx->loc_line[0] = ctx->loc_line[1];
 ctx->loc_col[0] = ctx->loc_col[1];
#if 1
 ctx->strtree_state = UREGLEX_STRTREE_MORE;
 ctx->strtree_len = ctx->strtree_score = 0;
 ctx->strtree.ip = ctx->strtree.code = pcb_bxl_strings;
 ctx->sp = ctx->buff;
#endif
}
void pcb_bxl_lex_init(pcb_bxl_ureglex_t *ctx, ureglex_precomp_t *rules)
{
 ureglex_precomp_t *p;
 ctx->rules = rules;
 ctx->num_rules = 0;
 ctx->buff_save_term = ctx->step_back_to = -1;
 for(p = pcb_bxl_rules; p->nfa != NULL; p++)
  ctx->state[ctx->num_rules++].pc = p;
 ctx->by_len = (p->weight > 0.0);
 ctx->loc_offs[1] = ctx->loc_line[1] = ctx->loc_col[1] = 1;
 pcb_bxl_lex_reset(ctx);
 ctx->loc_offs[1] = ctx->loc_col[1] = 0;
}
int pcb_bxl_lex_char(pcb_bxl_ureglex_t *ctx, void *user_ctx, int chr)
{
 ureglex_t *best = NULL;
 int n, working = 0;
 if (ctx->buff_used >= (sizeof(ctx->buff)-1))
  return UREGLEX_TOO_LONG;
 ctx->buff[ctx->buff_used++] = chr;
 ctx->buff[ctx->buff_used] = '\0';
 ctx->loc_offs[1]++;
 if (chr == '\n') {
  ctx->loc_line[1]++;
  ctx->loc_col[1] = 0;
 }
 else
  ctx->loc_col[1]++;
#if 1
 while((ctx->strtree_state == UREGLEX_STRTREE_MORE) && (ctx->sp < (&ctx->buff[ctx->buff_used]))) {
  ctx->strtree_state = ureglex_strtree_exec(&ctx->strtree, *ctx->sp++);
  if (ctx->strtree_state == UREGLEX_STRTREE_MORE)
   working++;
  if (ctx->strtree_state > 0) {
   ureglex_t *s = &ctx->state[ctx->strtree_state];
   ctx->strtree_len = ctx->buff_used;
   s->exec_state = s->score = ctx->strtree_score = (int)((double)(ctx->strtree_len-1) * s->pc->weight * 100);
   s->bopat[0] = ctx->buff;
   s->eopat[0] = ctx->buff + ctx->strtree_len - 1;
  }
 }
#endif
 for(n = 0; n < ctx->num_rules; n++) {
  ureglex_t *s = &ctx->state[n];
  if ((s->pc->bittab != NULL) && (s->exec_state < 0)) {
   s->exec_state = ureglex_exec(s);
   if (s->exec_state < 0)
    working++;
   else if ((s->exec_state > 0) && (s->pc->weight != 1.0))
    s->exec_state = s->score = (int)((double)s->exec_state * s->pc->weight);
  }
 }
 if (working != 0)
  return UREGLEX_MORE;
 for(n = 0; n < ctx->num_rules; n++) {
  ureglex_t *s = &ctx->state[n];
  if ((s->pc->bittab != NULL) && (s->exec_state > 0)) {
   if (best == NULL)
    best = s;
   else if ((!ctx->by_len) && (s->score > best->score))
    best = s;
   else if ((ctx->by_len) && (s->eopat[0] - s->eopat[1] > best->eopat[0] - best->eopat[1]))
    best = s;
  }
 }
#if 1
 if (ctx->strtree_state > 0) {
  if (best == NULL)
   best = &ctx->state[ctx->strtree_state];
  else if ((!ctx->by_len) && (ctx->strtree_score > best->score))
   best = &ctx->state[ctx->strtree_state];
  else if ((ctx->by_len) && (ctx->strtree_len > best->eopat[0] - best->eopat[1]))
   best = &ctx->state[ctx->strtree_state];
 }
#endif
 if (best == NULL) {
  ctx->step_back_to = ctx->buff_used-1;
  return UREGLEX_NO_MATCH;
 }
 ctx->step_back_to = best->eopat[0] - ctx->buff;
 ctx->buff_save_term = ctx->buff[ctx->step_back_to];
 ctx->buff[ctx->step_back_to] = '\0';
 return pcb_bxl_user_code(ctx, user_ctx, best - ctx->state);
}
/* exec_spec.c END } */

