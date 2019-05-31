/* includeed from act_read.c */

static const char pcb_acts_IDPList[] =
	"IDPList(new|free|print)\n"
	"IDPList(get|print, idx)\n"
	"IDPList(remove, idx)\n"
	"IDPList(prepend|append, idpath)"
	;
static const char pcb_acth_IDPList[] = "Basic idpath list manipulation.";
static fgw_error_t pcb_act_IDPList(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return -1;
}
