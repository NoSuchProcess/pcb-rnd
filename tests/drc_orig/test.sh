#!/bin/sh

fail=0

for n in *.lht
do
	bn=${n%%.lht}
	./drc.sh $n > $bn.out
	diff -u $bn.ref $bn.out
	if test $? = 0
	then
		rm $bn.out
	else
		fail=1
	fi
done


exit $fail
