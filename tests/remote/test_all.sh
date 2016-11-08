#!/bin/sh
fail=0
all=0
for ref in *.ref
do
	bn=${ref%%.ref}
	./test_parse < $bn.msg > $bn.out
	diff -u $bn.ref $bn.out
	if test $? = 0
	then
		rm $bn.out
	else
		fail=$(($fail+1))
	fi
	all=$(($all+1))
done

if test $fail = 0
then
	echo "*** QC PASS ***"
else
	echo "FAIL: $fail of $all"
fi
