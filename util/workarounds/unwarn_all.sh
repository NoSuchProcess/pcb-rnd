#!/bin/sh

# This file is placed in the Public Domain.

# Comment all #warnings in all .h and .c files, recursively.
# Useful on systems with CC with no support for #warning.

action=`echo "$0" | sed "s/_all//"`

find . -name '*.[ch]' -exec $action {} \;
