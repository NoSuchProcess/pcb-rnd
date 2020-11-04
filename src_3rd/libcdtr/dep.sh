#!/bin/sh

cflags="$@"
tr " " "\n" | while read obj
do
	c=${obj%%.o}.c
	gcc -MT $obj -MM $c $cflags
done
