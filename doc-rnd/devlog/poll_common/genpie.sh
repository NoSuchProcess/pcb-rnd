#!/bin/sh

gen()
{
	awk -F "[\t]" -v "col=$1" '
	BEGIN {
		if (col < 0) {
			col = -col
			keep_na = 1
		}
	}
	{
		val=$col
		if (val == "-") {
			if (keep_na)
				val="n/a"
			else
				next
		}
		split(val, VAL, "[+]")
		for(n in VAL) {
			VOTE[VAL[n]]++
		}
	}

	END {
		for(n in VOTE) {
			print "@slice\n" VOTE[n]
			print "@label\n" n
		}
	}
	'
}

for n in $*
do
	(gen $n < poll.tsv | animpie && echo "screenshot \"pie_col_$n.png\"") | animator -H -x 150 -y 150
done
