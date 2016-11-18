#!/bin/sh
echo "Renaming action $1" >&2

na=`echo $1 | sed "s/^Action//"`
lc=`echo $na | tr "[A-Z]" "[a-z]"`

./renameo.sh "$1"            "pcb_act_$na"
./renameo.sh "${lc}_help"    "pcb_acth_$na"
./renameo.sh "${lc}_syntax"  "pcb_acts_$na"
./rename.sh "s/\\([^a-zA-Z0-9_]\\)AFAIL[(]${lc}[)]/\\1PCB_ACT_FAIL($na)/g"
