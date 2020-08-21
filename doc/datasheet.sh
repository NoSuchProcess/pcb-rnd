#!/bin/sh

. ../util/devhelpers/awk_on_formats.sh

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
