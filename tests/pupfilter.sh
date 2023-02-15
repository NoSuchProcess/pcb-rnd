#!/bin/sh

# ignore plugin loading errors: this way we do not need to install all
# librnd deps

exec awk -v "how=$1" '
	BEGIN {
		ws = how ~ "ws";
	}
	/^E: Some of the dynamic/ { next }
	/^E: puplug:/ { next}
	/^pcb-rnd conf ERROR: conf node .*not unregistered/ { next }
	/^ERROR: hid_actions_uninit: action.*left registered/ { next }
	/^pcb_plug_fp_chain is not empty;/  { next }
	/^... Log produced during uninitialization:/ { next }
	/some plugins are not soft-unloaded/ { next }
	(ws && (/^[ \t]*$/)) { next }
	{ print $0 }
'
