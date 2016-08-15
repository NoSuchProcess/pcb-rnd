#!/bin/sh
svn log -r$1:HEAD | grep -v "^\(-*$\|r[0-9]\+\)"
