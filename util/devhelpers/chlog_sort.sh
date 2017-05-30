#!/bin/sh

# read the outpot of chlog.sh and split it per topic and save the lines
# into changelog-formatted files

awk '
	BEGIN {
		IGNORE["todo"]=1
		IGNORE["devlog"]=1
		IGNORE["bugreport"]=1
		IGNORE["blog_queue"]=1
	}
	
	($1 ~ "^[[][^]]*\]$") {
		tag=tolower($1)
		sub("[[]", "", tag)
		sub("\]", "", tag)

		if (tag in IGNORE)
			next

		$1=""
		line=$0
		sub("^[ \t]*", "		", line)
		if (!(tag in SEEN)) {
			print "	[" tag "]" > "CHG." tag
			SEEN[tag]++
		}
		print line > "CHG." tag
		next
	}
	{
		line=$0
		sub("^[ \t]*", "		", line)
		print line > "CHG.MISC"
	}
'
