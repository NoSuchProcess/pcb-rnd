typedef struct pcb_qry_node_s pcb_qry_node_t;
typedef enum {
	PCBQ_EXPR,
	PCBQ_OP_AND,
	PCBQ_OP_OR,
	PCBQ_OP_EQ,
	PCBQ_OP_NEQ,
	PCBQ_OP_GTEQ,
	PCBQ_OP_LTEQ,
	PCBQ_OP_GT,
	PCBQ_OP_LT,
	PCBQ_OP_ADD,
	PCBQ_OP_SUB,
	PCBQ_OP_MUL,
	PCBQ_OP_DIV,
	PCBQ_DATA_COORD,   /* leaf */
	PCBQ_DATA_DOUBLE,  /* leaf */
	PCBQ_DATA_STRING,  /* leaf */

	PCBQ_nodetype_max
} pcb_qry_nodetype_t;

struct pcb_qry_node_s {
	pcb_qry_nodetype_t type;
	pcb_qry_node_t *next;       /* sibling on this level of the tree (or NULL) */
	union {
		Coord crd;
		double dbl;
		const char *str;
		pcb_qry_node_t *children;   /* first child (NULL for a leaf node) */
	};
};

pcb_qry_node_t *pcb_qry_n_alloc(pcb_qry_nodetype_t ntype);
pcb_qry_node_t *pcb_qry_n_insert(pcb_qry_node_t *parent, pcb_qry_node_t *ch);





