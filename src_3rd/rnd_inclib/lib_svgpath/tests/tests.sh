#!/bin/sh

bad=0
for n in *.in
do
	bn=${n%%.in}
	echo -n "$bn... "
	out=$bn.svg
	ref=$bn.ref
	./tester < $n > $out
	diff -u $ref $out
	if test $? = 0
	then
		echo "ok"
		rm $out
	else
		echo "!! BROKEN !!"
		bad=1
	fi
done

if test $bad = 0
then
	echo ""
	echo "QC PASS"
	echo ""
fi
