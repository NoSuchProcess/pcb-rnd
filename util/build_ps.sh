#!/bin/sh

# convert an ordered list of html documents into ps, using .png versions of
# .svg images, inserting a page break between any two files.

(for n in "$@"
do
	echo "<!--NewPage-->";
	bn1=`dirname $n`
	for svg in `ls $bn1/*.svg 2>/dev/null`
	do
		png=${svg%%.svg}.png
		if test ! -f $png
		then
			echo "Converting $svg to $png..." >&2
			convert $svg $png
		fi
	done
	sed "$HMTL2PS_SED;s/\.svg/.png/g;s@src=\"@src=\"$bn1/@g" $n
done) | tee HTML2PS.html | html2ps $HTML2PS_OPTS --colour

echo html2ps $HTML2PS_OPTS --colour >&2
