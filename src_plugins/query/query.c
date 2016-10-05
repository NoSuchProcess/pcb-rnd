#include "config.h"
#include "global.h"
#include "conf.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "query.h"

/******** tree helper ********/

const char *type_name[PCBQ_nodetype_max] = {
	"PCBQ_EXPR",
	"PCBQ_OP_AND",
	"PCBQ_OP_OR",
	"PCBQ_OP_EQ",
	"PCBQ_OP_NEQ",
	"PCBQ_OP_GTEQ",
	"PCBQ_OP_LTEQ",
	"PCBQ_OP_GT",
	"PCBQ_OP_LT",
	"PCBQ_OP_ADD",
	"PCBQ_OP_SUB",
	"PCBQ_OP_MUL",
	"PCBQ_OP_DIV",
	"PCBQ_DATA_COORD",
	"PCBQ_DATA_DOUBLE",
	"PCBQ_DATA_STRING"
};

const char *pcb_qry_nodetype_name(pcb_qry_nodetype_t ntype)
{
	int type = ntype;
	if ((type < 0) || (type >= PCBQ_nodetype_max))
		return "<invalid>";
	return type_name[type];
}

pcb_qry_node_t *pcb_qry_n_alloc(pcb_qry_nodetype_t ntype)
{
	pcb_qry_node_t *nd = calloc(sizeof(pcb_qry_node_t), 1);
	nd->type = ntype;
	return nd;
}

pcb_qry_node_t *pcb_qry_n_insert(pcb_qry_node_t *parent, pcb_qry_node_t *ch)
{
	ch->next = parent->children;
	parent->children = ch;
	return parent;
}

static char ind[] = "                                                                                ";
void pcb_qry_dump_tree_(int level, pcb_qry_node_t *nd)
{
	pcb_qry_node_t *n;
	if (level < sizeof(ind))  ind[level] = '\0';
	printf("%s%s\n", ind,  pcb_qry_nodetype_name(nd->type));
	switch(nd->type) {
		case PCBQ_DATA_COORD:  pcb_printf("%s %mI (%$mm)\n", ind, nd->crd, nd->crd); break;
		case PCBQ_DATA_DOUBLE: pcb_printf("%s %f\n", ind, nd->dbl); break;
		case PCBQ_DATA_STRING: pcb_printf("%s '%s'\n", ind, nd->str); break;
		default:
			if (level < sizeof(ind))  ind[level] = ' ';
			for(n = nd->children; n != NULL; n = n->next)
				pcb_qry_dump_tree_(level+1, n);
			return;
	}
	if (level < sizeof(ind))  ind[level] = ' ';
}

pcb_qry_dump_tree(pcb_qry_node_t *top)
{
	pcb_qry_dump_tree_(0, top);
}

/******** parser helper ********/
void qry_error(const char *err)
{
	pcb_trace("qry_error: %s\n", err);
}

int qry_wrap()
{
	return 1;
}

/******** plugin helper ********/
void query_action_reg(const char *cookie);

static const char *query_cookie = "query plugin";

static void hid_query_uninit(void)
{
	hid_remove_actions_by_cookie(query_cookie);
}

pcb_uninit_t hid_query_init(void)
{
	query_action_reg(query_cookie);
	return hid_query_uninit;
}
