#!/bin/sh

# convert an ordered list of html documents into ps, using .png versions of
# .svg images, inserting a page break between any two files.

(for n in "$@"
do
	echo "<!--NewPage-->";
	if test -z $HTML2PS_SED
	then
		cat $n
	else
		sed "$HMTL2PS_SED" $n
	fi
done) | sed "s/\.svg/.png/g" | html2ps $HTML2PS_OPTS --colour

echo html2ps $HTML2PS_OPTS --colour >&2
