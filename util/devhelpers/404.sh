#!/bin/sh

# search the official web page for broken links

echo "Collecting the log..." >&2
wget --spider -o 404.raw.log -r -p http://repo.hu/projects/pcb-rnd

echo "Evaluatng the log..." >&2
awk '
	function report() {
		print url
		rerorted = 1
	}

	/^--/ {
		url=$0
		sub("--.*--  *", "", url)
		sub("^[(]try[^)]*[)] *", "", url)
	}
	/^$/ { url = ""; reported = 0 }
	(reported) { next }
	/response.*404/ { report() }
	/broken link/ { report() }
' < 404.raw.log | tee 404.log
