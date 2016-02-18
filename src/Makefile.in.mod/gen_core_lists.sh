#!/bin/sh
for f in $*
do
	echo "___name___ $f"
	cat $f
done | awk '
BEGIN {
	q = "\""
}

/^___name___/ {
	fullname = $2
	basename = $2
	sub("/[^/]*$", "", basename)
	if (fullname ~ "src_plugins/")
		TYPE[basename]="plugdir"
}

/^type=/ {
	type = $0
	sub("^type=", "", type)
	TYPE[basename] = type
	print "/* ", type, basename " */"
}


/^REGISTER/ {
	LIST[basename] = LIST[basename] $0 "\n"
}

END {
	for(n in LIST) {
		hn = n
		sub("^hid/", "", hn)
#		if (hn in HIDNAME_FIXUP)
#			hn = HIDNAME_FIXUP[hn]

		print "/* " n  " (" TYPE[n] ") */"
		if (TYPE[n] == "gui")
			print "if ((gui != NULL) && (strcmp(gui->name, " q hn q ") == 0)) {"
		if (TYPE[n] == "plugdir") {
			vname = LIST[n]
			sub("REGISTER_ACTIONS.*[(]", "", vname)
			sub("[)].*[\n\r]*", "", vname)
			print "extern HID_Action " vname "[];"
		}
		print LIST[n]
		if (TYPE[n] == "gui")
			print "}"
	}
}

'

