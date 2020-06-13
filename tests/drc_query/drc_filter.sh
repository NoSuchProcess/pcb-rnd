#!/bin/sh

# make drc output comparable (remove dependency on order of reports)

awk '

BEGIN {
	shift = 10000
	tn = 0;
}

function append(line)
{
	if (text == "")
		text = line
	else
		text = text "\n" line
}

function fin()
{
	if (text != "") {
		TEXT[tn] = text;
		HASH[tn] = hash+1;
		tn++;
		text = "";
		hash = 0;
	}
}

function print_best(        n,best,bestn)
{
	best = 0
	for(n = 0; n < tn; n++) {
		if (TEXT[n] == "")
			continue;
		if (HASH[n] > best) {
			best = HASH[n]
			bestn = n
		}
	}

	if (bestn == "")
		return 0;

	print TEXT[bestn]
	TEXT[bestn] = ""
	return 1;
}

function print_all()
{
	while(print_best());
}

#1: object beyond drawing area: Objects located outside of the drawing area
/^[0-9]+:/ {
	line=$0;
	sub("^[0-9]+:", "x:", line);
	if (!(line in ID)) {
		ids++;
		ID[line] = ids;
	}
	hash = ID[line] * shift * shift;
	append(line);
	next
}

#within (112.40, -106.35, 237.60, -43.65) mil
/^within/ {
	line=$0;
	sub(".*[(]", "", line);
	split(line, C, "[,]");

	hash += C[1] * shift + C[2]
}

{ append($0); }

/^$/ { fin() }

END {
	fin();
	print_all();
}


'
