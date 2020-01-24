#!/bin/sh

if test -z "$AWK"
then
	AWK="awk"
fi

for f in $*
do
	echo "___name___ $f"
	cat $f
done | $AWK '
BEGIN {
	q = "\""
	print "/**** DO NOT EDIT - automatically generated by gen_core_lists.sh ****/"
	print ""
}

/^___name___/ {
	basename = $2
	sub("/[^/]*$", "", basename)
}

/^PCB_REGISTER/ {
	LIST[basename] = LIST[basename] $0 "\n"
}

END {
	for(n in LIST) {
		print "/* " n " */"
		gsub("PCB_REGISTER_ACTIONS_FUNC", "PCB_REGISTER_ACTIONS_CALL", LIST[n])

		print LIST[n]
	}
}

'

