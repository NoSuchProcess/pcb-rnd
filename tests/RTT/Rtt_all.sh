#!/bin/sh

ulimit -c unlimited
rm core
all=`ls *.pcb | grep -v "[.]new[.]pcb$\|[.]orig[.]pcb$\|^[.]"`
num_all=`echo "$all" | wc -l`

cnt=0
for n in $all
do
	cnt=$(($cnt+1))
	echo "----------------------$n"
	echo "----------------------$n $cnt/$num_all" >&2
	./Rtt.sh $n "$@"
done > All.log

