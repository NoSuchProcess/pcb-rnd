#!/bin/sh

# This file is placed in the Public Domain.

# Comment all #warnings in a file given as $1.
# Useful on systems with CC with no support for #warning.

sed '
	/^#[ \t]*warning.*/ {
		s@^@TODO("@
		s@$@")@
		s@ *TODO:@:@
		s@#[ \t]*warning *@:@
	}
' < "$1" > "$1.tmp" && mv "$1.tmp" "$1"
