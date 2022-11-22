#!/bin/sh

# collates the pcb-rnd action table into a html doc page

asrc="../action_src"
lsrc="librnd_acts"

. $LIBRND_LIBDIR/dump_actions_to_html.sh

cd ../../../../src
pcb_rnd_ver=`./pcb-rnd --version`
pcb_rnd_rev=`svn info ^/ | awk '/Revision:/ {
	print $0
	got_rev=1
	exit
	}
	END {
		if (!got_rev)
		print "Rev unknown"
	}
	'`
cd ../doc/user/09_appendix/src

print_hdr "pcb-rnd"

(
	cd ../../../../src
	./pcb-rnd --dump-actions 2>/dev/null
) | gen
