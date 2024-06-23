#!/bin/sh
for n in *.in
do
	bn=${n%%.in}
	out=$bn.svg
	ref=$bn.ref
	./tester < $n > $out
	diff -u $ref $out
done