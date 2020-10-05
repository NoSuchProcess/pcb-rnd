#!/bin/sh

SRC=../../src
CFG="-c rc/library_search_paths=. -c rc/quiet=1"

#	MenuPatch(load, test, "insert.lht");

cp $SRC/default_font $SRC/default4.lht .
rm d0.lht d1.lht >/dev/null 2>&1

echo '
	MenuDebug(force-enable);
	MenuDebug(save, "d0.lht");
'$1'
	MenuDebug(save, "d1.lht");
' | $SRC/pcb-rnd $CFG -c rc/menu_file=base --gui batch 2>&1 | grep -v "^menu debug"

diff -U1 d0.lht d1.lht | grep "^[ +-][^+-]"

rm d0.lht d1.lht default_font default4.lht >/dev/null 2>&1