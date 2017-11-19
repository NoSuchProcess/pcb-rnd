#!/bin/sh

URL=svn://repo.hu/pcb-rnd
#URL=svn://10.0.0.3/pcb-rnd

#tmp=`mktemp -d`
tmp=/tmp/chgstat_graph
export trunk="$tmp/trunk"
mkdir -p $tmp

for year in 2015 2016 2017
do
	for month in 1 2 3 4 5 6 7 8 9 10 11 12
	do
		svn checkout "$URL/trunk" "$trunk"  -r "{$year-$month-01}" >&2
		echo "$year-$month"
		./chgstat.sh
		rm `find $trunk -name '*.blm'`
	done
done | tee -a CHG.GR

