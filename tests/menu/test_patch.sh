#!/bin/sh

ROOT=../..
SRC=$ROOT/src
CFG="-c rc/library_search_paths=dummy_lib -c rc/quiet=1"

if test -x $SRC/pcb-rnd.wrap
then
	PCB_RND=./pcb-rnd.wrap
else
	PCB_RND=./pcb-rnd
fi


# call:
# insert.lht 'MenuPatch(load, test, "insert.lht");'

rm d0.lht d1.lht >/dev/null 2>&1

cp menu-base.lht $1 $SRC
(
cd $SRC
mkdir dummy_lib
echo '
	MenuDebug(force-enable);
	MenuDebug(save, "d0.lht");
'$2'
	MenuDebug(save, "d1.lht");
' | $PCB_RND $CFG -c rc/menu_file=base --gui batch 2>&1 | grep -v "^menu debug"
rmdir dummy_lib
)

mv $SRC/d0.lht .
mv $SRC/d1.lht .
rm $SRC/menu-base.lht $SRC/$1

diff -U1 d0.lht d1.lht | grep "^[ +-][^+-]"

rm d0.lht d1.lht >/dev/null 2>&1
