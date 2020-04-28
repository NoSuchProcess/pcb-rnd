#include <libminuid/libminuid.h>
#include <librnd/core/compat_misc.h>
#include "pcb_minuid.h"

minuid_session_t pcb_minuid;


void pcb_minuid_init(void)
{
	int pid = rnd_getpid();
	minuid_init(&pcb_minuid);
	minuid_salt(&pcb_minuid, &pid, sizeof(pid));
}

