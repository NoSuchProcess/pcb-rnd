#!/bin/sh

# Rename a function and log

./rename.sh "s/\\([^a-zA-Z0-9_>.]\\)${1}[ \t]*[(]/\\1$2(/g"
./rename.sh "s/^${1}[ \t]*[(]/$2(/g"

#echo "#define $1 $2" >> src/librnd/pcb_compat.h
