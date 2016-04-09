#!/bin/sh
sed "s@\t@,@g" < poll.tsv > poll.csv
tr "\n" "," < header > header.csv
