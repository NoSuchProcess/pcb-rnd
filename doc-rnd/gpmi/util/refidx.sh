#!/bin/sh

idx()
{
./tags < $1 | awk -v "fn=$1" '
/^<[Hh]3>/ {
	sub("^<[Hh]3>[ \t]*", "", $0)
	type=tolower($1)
	if ((type != "event") && (type != "function"))
		next
	name=$0
	sub("[(].*", "", name)
	sub("^.*[ \t*]", "", name)
	print type, name, fn
}
'
}

for n in ../packages/*.html
do
	idx $n
done