#ifndef PCB_ORDER_CONSTRAINT
#define PCB_ORDER_CONSTRAINT

typedef enum {
	PCB_ORDC_IF,
	PCB_ORDC_ERROR,

	PCB_ORDC_INTEGER,
	PCB_ORDC_FLOAT,
	PCB_ORDC_STRING,

	PCB_ORDC_NEG,
	PCB_ORDC_EQ,
	PCB_ORDC_NEQ,
	PCB_ORDC_GE,
	PCB_ORDC_LE,
	PCB_ORDC_GT,
	PCB_ORDC_LT,
	PCB_ORDC_ADD,
	PCB_ORDC_SUB
} pcb_ordc_node_type_t;

typedef struct pcb_ordc_node_s pcb_ordc_node_t;
struct pcb_ordc_node_s {
	pcb_ordc_node_type_t type;
	union {
		struct {
			pcb_ordc_node_t *o1, *o2;
		} op;
		union {
			long i;
			double d;
			char *s;
		} data;
	} val;
};

typedef struct pcb_ordc_ctx_s {
	int dummy;
	pcb_ordc_node_t *root;
} pcb_ordc_ctx_t;

int pcb_ordc_parse_str(pcb_ordc_ctx_t *ctx, const char *script);


#endif
