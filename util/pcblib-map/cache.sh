#!/bin/sh

digest()
{
	local fn bn
	fn=$1

	bn=${fn#./}

	if test -d "$fn"
	then
		echo "D $bn"
		return
	fi

	prm=`grep "@@purpose" $fn`
	if test -z "$prm"
	then
		tags=`awk '
		/^##/ {
			tag=$0
			sub("^##","", tag)
			printf " %s", tag
		}
		' < $fn`
		echo "S $bn $tags"
	else
		tags=`awk '
		/^@@tag/ {
			tag=$0
			sub("^@@tag *","", tag)
			printf " %s", tag
		}
		' < $fn`
		echo "P $bn $tags"
	fi

}

(
cd ../../pcblib
for n in `find .`
do
	case $n in
		*.svn*) ;;
		.) ;;
		*)
			digest $n
	esac
done
)
