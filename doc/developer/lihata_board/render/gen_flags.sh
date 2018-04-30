../../../../src/pcb-rnd --dump-oflags | awk '
BEGIN {
	print "ha:lht_tree_doc { ha:comm {"
}

function close_last()
{
	if (is_open) {
		print "		}"
		print "	}"
	}
	is_open = 0;
}

/^[^\t]/ {
	close_last()
	print "	ha:flags_" $1 " {"
	print "		hide=1"
	print "		desc={flag bits of a " $1 "}"
	print "		type=ha"
	print "		li:children {"
	is_open=1
}

/^[\t]/ {
	bit=$1
	name=$2
	help=$0
	sub("^[\t ]*" $1 "[\t ]*" $2 "[\t ]*", "", help)
	print "			ha:" name " { valtype=flag; desc={" help "}}"
}

END {
	close_last()
	print "} }"
}

'
