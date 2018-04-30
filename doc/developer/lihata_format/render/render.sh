#!/bin/sh

for n in ../*.lht
do
	lhtflat < $n
done | tee Flat | awk -F "[\t]" -f lht.awk -f common.awk -f $1.awk
