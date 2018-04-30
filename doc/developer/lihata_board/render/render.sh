#!/bin/sh

echo '
<html>
<body>
'

for n in ../*.lht
do
	lhtflat < $n
done | tee Flat | awk -F "[\t]" -f lht.awk -f html.awk

echo '
</body>
</html>
'
