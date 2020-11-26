#include "undo.h"
#include "globalconst.h"

/* Live scripting needs to undo using the host app; to make this host app
   API independent, the action interface should be used */

static long get_undo_serial(rnd_hidlib_t *hl)
{
	return pcb_undo_serial();
}

static long get_num_undo(rnd_hidlib_t *hl)
{
	return pcb_num_undo();
}

static void inc_undo_serial(rnd_hidlib_t *hl)
{
	pcb_undo_inc_serial();
}

static void undo_above(rnd_hidlib_t *hl, long ser)
{
	pcb_undo_above(ser);
}
