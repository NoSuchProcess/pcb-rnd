#!/bin/sh
# dump remote HID communication

echo '
	MakeGC(1)
	Ready()
	MakeGC(2)
	MakeGC(3)
	MakeGC(4)
' | pcb-rnd --gui remote $1  | grep -v "^[A-Z]:" > out/${1%%.pcb}.remote
rm -f out/${1%%.pcb}.remote.gz
gzip out/${1%%.pcb}.remote
