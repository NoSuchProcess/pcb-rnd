#!/bin/sh

# ignore plugin loading errors: this way we do not need to install all
# librnd deps

exec awk '
	/^E: Some of the dynamic/ { next }
	/^E: puplug:/ { next}
	{ print $0 }
'
