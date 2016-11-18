#!/bin/sh

./rename.sh "
	s/\\([^a-zA-Z0-9_]\\)${1}\\([^a-zA-Z0-9_]\\)/\\1$2\\2/g;
	s/^${1}\\([^a-zA-Z0-9_]\\)/$2\\1/g;
	s/\\([^a-zA-Z0-9_]\\)${1}$/\\1$2/g;
"

echo "$1 -> $2" >> doc-rnd/hacking/renames
