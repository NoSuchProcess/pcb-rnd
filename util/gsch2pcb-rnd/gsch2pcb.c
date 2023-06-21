#include <stdio.h>

#define NL "\n"

static const char *oops[] = {
	NL, NL, NL,
	NL "gsch2pcb-rnd has been deprecated and removed from pcb-rnd.",
	NL "If you see this message:",
	NL,
	NL "1. Please consider switching to the import schematics workflow, using",
	NL "   the lepton or the gnetlist (for gschem) format; see file menu, import,",
	NL "   import schematics submenu. For the GUI usage, see first half of:",
	NL "   http://repo.hu/cgi-bin/pool.cgi?project=pcb-rnd&cmd=show&node=isch_switch",
	NL,
	NL "2. If you are using gsch2pcb-rnd from a shell script or Makefile:",
	NL "   import schematics can be automated too, no GUI involved. See second",
	NL "   half of:",
	NL "   http://repo.hu/cgi-bin/pool.cgi?project=pcb-rnd&cmd=show&node=isch_switch",
	NL,
	NL "3. Alternatively: get lepton-netlist or gnetlist to export a tEDAx",
	NL "   netlist (both supports this format natively for years already) and",
	NL "   load that in pcb-rnd (either from GUI or calling pcb-rnd with --hid batch",
	NL "   and using a few commands from stdin)",
	NL,
	NL "4. If none of the above worked for some reason, please contact the",
	NL "   developers to get free help/support with the switchover:",
	NL "    - use the \"HELP on-line support\" button (blue, top right in pcb-rnd)",
	NL "    - send a mail to the mailing list",
	NL "    - send a mail to the lead developer: http://igor2.repo.hu/contact.html",
	NL,
	NL "5. If that failed too, you can install the latest, unmaintained version",
	NL "   of gsch2pcb-rnd from source: http://www.repo.hu/projects/gsch2pcb-rnd",
	NL,
	NL "You may also be interested in sch-rnd: http://www.repo.hu/projects/sch-rnd",
	NL "which is the schematics editor of Ringdove, very similar to pcb-rnd and"
	NL "can fully replace lepton-eda or gschem."
	NL, NL, NL,
	NULL
	};

int main(int argc, char *argv[])
{
	const char **s;
	for(s = oops; *s != NULL; s++)
		fputs(*s, stderr);
	return 1;
}
