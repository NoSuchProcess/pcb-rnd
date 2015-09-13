#include "rats_patch.h"

void rats_patch_append(PCBTypePtr pcb, rats_patch_op_t op, const char *id, const char *a1, const char *a2)
{
	rats_patch_line_t *n;

	n = malloc(sizeof(rats_patch_line_t));

	n->op = op;
	n->id = strdup(id);
	n->arg1.net_name = strdup(a1);
	if (a2 != NULL)
		n->arg2.attrib_val = strdup(a2);
	else
		n->arg2.attrib_val = NULL;

	if (pcb->NetlistPatches != NULL) {
		pcb->NetlistPatchLast->next = n;
		pcb->NetlistPatchLast = n;
	}
	else
		pcb->NetlistPatchLast = pcb->NetlistPatches = n;
	n->next = NULL;
}

static void netlist_copy(LibraryTypePtr dst, LibraryTypePtr src)
{
	int n, p;
	dst->MenuN = src->MenuN;
	dst->MenuMax = dst->Menu = calloc(sizeof(LibraryMenuType), dst->MenuN);
	for (n = 0; n < src->MenuN; n++) {
		LibraryMenuTypePtr smenu = &src->Menu[n];
		LibraryMenuTypePtr dmenu = &dst->Menu[n];
/*		fprintf(stderr, "Net %d name='%s': ", n, smenu->Name+2);*/
		dmenu->Name = strdup(smenu->Name);
		dmenu->EntryMax = dmenu->EntryN = smenu->EntryN;
		dmenu->Entry = calloc(sizeof(LibraryEntryType), dmenu->EntryN);
		dmenu->flag = smenu->flag;
		dmenu->internal = smenu->internal;
		for (p = 0; p < smenu->EntryN; p++) {
			LibraryEntryTypePtr sentry = &smenu->Entry[p];
			LibraryEntryTypePtr dentry = &dmenu->Entry[p];
/*			fprintf (stderr, " '%s'", sentry->ListEntry);*/
			dentry->ListEntry = strdup(sentry->ListEntry);
		}
/*		fprintf(stderr, "\n");*/
	}
}

void rats_patch_apply(PCBTypePtr pcb)
{
	/* netlist_free(&(PCB->NetlistLib[NETLIST_EDITED])) */
	netlist_copy(&(pcb->NetlistLib[NETLIST_EDITED]), &(pcb->NetlistLib[NETLIST_INPUT]));
}
