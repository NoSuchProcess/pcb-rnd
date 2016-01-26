#!/bin/sh
(
echo '<html><body><table border=1>'
echo "<tr>"
sed "s@^@<th>@" < header
sed "s@^@<tr><td>@;s@\t@<td>@g" < poll.tsv
echo '</table></body></html>'
) > poll.html

