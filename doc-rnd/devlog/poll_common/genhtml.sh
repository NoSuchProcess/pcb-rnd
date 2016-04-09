#!/bin/sh
(
echo '<html><body><table border=1>'
echo "<tr>"
sed "s@^@<th>@" < header
sed "s@^@<tr><td>@;s@\t@<td>@g" < poll.tsv
echo "<tr>"
hl=`wc -l < header`
for n in `seq $hl`
do
	./genpie.sh $n
	fn=pie_col_$n.png
	if test -f "$fn"
	then
		echo "<td><img src=\"$fn\">"
	else
		echo "<td>&nbsp;"
	fi
done

echo '</table>'
if test -f "legend.txt"
then
	echo "<p>"
	echo "<h3> Legend </h3>"
	echo "<pre>"
	cat legend.txt
	echo "</pre>"
fi

echo "<h3> Downloads </h3>"
echo '<ul>'

if test -f poll.csv
then
	echo '	<li><a href="poll.csv"> csv data </a> | <a href="header.csv"> csv header </a>'
fi

if test -f poll.tsv
then
	echo '	<li><a href="poll.tsv"> tsv data </a> (raw, no header)'
fi

if test -f legend.txt
then
	echo '	<li><a href="legend.txt"> legend </a>'
fi

echo '<ul>'

echo '</body></html>'
) > poll.html


