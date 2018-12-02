#!/bin/sh
files=`find ../.. -name *.[chly]`
grep "^[ \t]*TODO[(]" $files
