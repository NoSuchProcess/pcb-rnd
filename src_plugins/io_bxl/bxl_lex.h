/* strtree.h BEGIN { */
#ifndef UREGLEX_STRTREE_H
#define UREGLEX_STRTREE_H 
typedef enum {ULX_REQ = 1, ULX_BRA, ULX_FIN, ULX_BAD} ureglex_stree_op_t;
typedef struct ureglex_strtree_s { int *code, *ip; } ureglex_strtree_t;
int ureglex_strtree_exec(ureglex_strtree_t *ctx, int chr);
#define UREGLEX_STRTREE_MORE -5
#endif
/* strtree.h END } */

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
#define ULX_BUF ctx->buff
#define ULX_TAGP(n) (ctx->state[ruleid].bopat[(n)])
#define ULX_TAGL(n) (ctx->state[ruleid].eopat[(n)] - ctx->state[ruleid].bopat[(n)])
#define ULX_IGNORE goto ureglex_ignore;
#endif
/* exec.h END } */

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

/* exec_spec.h BEGIN { */
void pcb_bxl_lex_reset(pcb_bxl_ureglex_t *ctx);
void pcb_bxl_lex_init(pcb_bxl_ureglex_t *ctx, ureglex_precomp_t *rules);
int pcb_bxl_lex_char(pcb_bxl_ureglex_t *ctx, void *user_ctx, int chr);
/* exec_spec.h END } */

#ifndef URELGLEX_EXEC_pcb_bxl_H
#define URELGLEX_EXEC_pcb_bxl_H
extern ureglex_precomp_t pcb_bxl_rules[];
#define URELGLEX_EXEC_pcb_bxl_HAS_COMMON 1
#endif
