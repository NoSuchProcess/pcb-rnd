#!/bin/sh

# local wrapper around pcb-rtrip so it all can be executed from the source
# tree, without installation

TRUNK=../..
pcb_rnd_bin="$TRUNK/src/pcb-rnd"
RTRIP="$TRUNK/util/devhelpers/pcb-rtrip"

(
	. $RTRIP
	if test -f core
	then
		mv core "$fn.core"
	fi
)  2>&1  | grep -v "PCBChanged\|readres=-1\|No PCB loaded\|pcb-gpmi hid is loaded\|has no font information, using default font\|WARNING.*is not uninited\|but first saves\|WARNING.*left registered\|gpmi dirs:"
