#!/bin/sh

# convert an ordered list of html documents into ps, using .png versions of
# .svg images, inserting a page break between any two files.

autotoc()
{
awk '
	function inclev(level   ,n,s) {
		NUM[level]++
		for(n = level+1; n < 9; n++)
			NUM[n] = ""

		for(n = 1; n <= level; n++)
			s = s NUM[n] "."
		return s
	}

	(match($0, "<[Hh][1-9][^>]*[>]")) {
		h=substr($0, RSTART, RLENGTH)

		level=h
		sub("..", "", level)
		sub("[^0-9].*$", "", level)

		if (h ~ "autotoc=.yes.") {
			sect=inclev(level)
			print substr($0, 0, RSTART+RLENGTH), sect, substr($0, RSTART+RLENGTH+1)
			next
		}
		else {
			num=substr($0, RSTART+RLENGTH)
			sub("^[	 ]*", "", num)
			sub("[	 ].*$", "", num)
			v = split(num, A, "[.]")
			if (A[1] == int(A[1])) {
				# learn number
				if (A[v] == "")
					v--
				if (v != level) {
					print "WARNING: wrong section numbering (expected h" v " got h" level "): " $0 > "/dev/stderr"
				}
				for(n = 1; n <= v; n++)
					NUM[n] = A[n]
			}
			else {
				print "WARNING: unnumbered section", $0 > "/dev/stderr"
			}
		}
	}

	{ print $0 }
'
}

### main ###

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
done) | autotoc | tee HTML2PS.html | html2ps $HTML2PS_OPTS --colour

echo html2ps $HTML2PS_OPTS --colour >&2
