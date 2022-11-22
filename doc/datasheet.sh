#!/bin/sh

ROOT=..

if test -z "$LIBRND_LIBDIR"
then
	# when not run from the Makefile
	LIBRND_LIBDIR=`cd developer/packaging && make -f librnd_root.mk libdir`
fi

proot=$ROOT/src_plugins
. $LIBRND_LIBDIR/devhelpers/awk_on_formats.sh

awk_on_formats  '
($1 == "<!--") && ($2 == "begin") && ($3 == "fmt") { ignore = 1; print $0; next }
($1 == "<!--") && ($2 == "end") && ($3 == "fmt") {
	print FMTS[$4, $5]
	ignore = 0;
	print $0
	next
}

(!ignore) { print $0 }
' < datasheet.html > datasheet2.html && mv datasheet2.html datasheet.html
