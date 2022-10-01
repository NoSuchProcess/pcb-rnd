#ifndef PCB_ORDER_CONSTRAINT
#define PCB_ORDER_CONSTRAINT

#include <stdio.h>

typedef enum {
	PCB_ORDC_BLOCK,

	PCB_ORDC_IF,
	PCB_ORDC_ERROR,

	/* consts and variables */
	PCB_ORDC_CINT,
	PCB_ORDC_CFLOAT,
	PCB_ORDC_QSTR,
	PCB_ORDC_ID,
	PCB_ORDC_VAR,

	/* casts */
	PCB_ORDC_INT,
	PCB_ORDC_FLOAT,
	PCB_ORDC_STRING,

	PCB_ORDC_NEG,
	PCB_ORDC_EQ,
	PCB_ORDC_NEQ,
	PCB_ORDC_GE,
	PCB_ORDC_LE,
	PCB_ORDC_GT,
	PCB_ORDC_LT,

	PCB_ORDC_AND,
	PCB_ORDC_OR,
	PCB_ORDC_NOT,

	PCB_ORDC_ADD,
	PCB_ORDC_SUB,
	PCB_ORDC_MULT,
	PCB_ORDC_DIV,
	PCB_ORDC_MOD
} pcb_ordc_node_type_t;

typedef struct pcb_ordc_node_s pcb_ordc_node_t;
struct pcb_ordc_node_s {
	pcb_ordc_node_type_t type;
	union {
		long l;
		double d;
		char *s;
	} val;
	pcb_ordc_node_t *ch_first, *next;
};

typedef struct pcb_ordc_ctx_s {
	int dummy;
	pcb_ordc_node_t *root;
} pcb_ordc_ctx_t;

int pcb_ordc_parse_str(pcb_ordc_ctx_t *ctx, const char *script);


void pcb_ordc_uninit(pcb_ordc_ctx_t *ctx);

void pcb_ordc_free_tree(pcb_ordc_ctx_t *ctx, pcb_ordc_node_t *node);


/*** For debug ***/
void pcb_ordc_print_tree(FILE *f, pcb_ordc_ctx_t *ctx, pcb_ordc_node_t *node, int indlev);


#endif
