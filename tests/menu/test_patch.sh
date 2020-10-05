#!/bin/sh

#	MenuPatch(load, test, "insert.lht");

rm d0.lht d1.lht >/dev/null 2>&1

echo '
	MenuDebug(force-enable);
	MenuDebug(save, "d0.lht");
'$1'
	MenuDebug(save, "d1.lht");
' | pcb-rnd -c rc/menu_file=base --gui batch 2>&1 | grep -v "^menu debug"

diff -U1 d0.lht d1.lht | grep "^[ +-][^+-]"

rm d0.lht d1.lht >/dev/null 2>&1
