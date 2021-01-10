#!/bin/sh
res=0

for tfn in *.cdt
do
	bfn=${tfn%%.cdt}
	../cdt_test < $tfn > $bfn.out 2>&1
	diff -u $bfn.ref $bfn.out
	if test $? != 0
	then
		res=1
	else
		rm $bfn.out
	fi
done

exit $res
