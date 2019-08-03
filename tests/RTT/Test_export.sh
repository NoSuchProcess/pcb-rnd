#!/bin/sh

# Run the export test on all formats that have refs _and_ are supported
# by our current pcb-rnd executable

tester="./Export.sh -t -a"

# disabled until the rewrite:
#IPC-D-356
# disabled until figuring the rotation:
#dsn
want='
bom
eps
ps
XY  2>/dev/null
png
gerber
svg 2>/dev/null
gcode
'

have=`./Export.sh  --list`

export fail=0
echo "$want" | while read fmt args
do
	if test ! -z "$fmt"
	then
		have_=`echo "$have"|grep "^$fmt$"`
		if test ! -z "$have_"
		then
			eval "$tester -f $fmt $args"
			if test "$?" -ne "0"
			then
				fail=1
			fi
		else
			echo "$fmt: SKIP (plugin not enabled)"
		fi
	fi

	# the only way to set the return value of the while()
	if test "$fail" -ne "0"
	then
		false
	else
		true
	fi
done
