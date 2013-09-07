#!/bin/sh
for f in $*
do
	cat $f
done | grep "^REGISTER"


