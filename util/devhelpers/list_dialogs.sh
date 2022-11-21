# shell lib

# assumes running from the source tree

# list all dialogs from the source files provided on the arg list
list_dlgs()
{
grep RND_DAD_NEW "$@" | awk '
	($1 ~ "TEMPLATE") { next }
	{
		file=$1
		name=$0
		sub(":$", "", file)
		sub(".*src/", "src/", file)
		sub(".*src_plugins/", "src_plugins/", file)
		sub(".*RND_DAD_NEW[^(]*[(]", "", name)
		title=name
		if (name ~ "^\"") {
			sub("\"", "", name)
			sub("\".*", "", name)
		}
		else
			name = "<dyn>"

		sub("[^,]*,[^,]*, *", "", title)
		if (title ~ "^\"") {
			sub("\"", "", title)
			sub("\".*", "", title)
		}
		else
			title = "<dyn>"

		print name "\t" title "\t" file
	}
'
}

# read dialog list and emit a html
gen_html()
{
awk -F "[\t]" '
function orna(s)
{
	if ((s == "") || (s == "<dyn>")) return "n/a"
	return s
}

'"$dlgtbl"'
'"$dlgextra"'

function out(id, name, src, action, comment     ,acturl1,acturl2,fn,tmp) {
	if (action == "") {
		if (id in ACTION) action = ACTION[id]
		else if (src in ACTION) action = ACTION[src]
	}

	if (action != "") {
		acturl1 = action
		sub("[(].*", "", acturl1)
		fn = "../action_src/" acturl1 ".html"
		if ((getline tmp < fn) == 1) {
			acturl1 = "<a href=\"action_details.html#" tolower(acturl1) "\">"
			acturl2 = "</a>"
		}
		else {
			acturl1 = ""
			acturl2 = ""
		}
		close(fn)
	}

	if (comment == "") {
		if (id in COMMENT) comment = COMMENT[id]
		else if (src in COMMENT) comment = COMMENT[src]
		else comment = "&nbsp;"
	}

	print "<tr><td>" orna(id) "<td>" orna(name) "<td>" acturl1 orna(action) acturl2 "<td>" src "<td>" comment
}

{
	id=$1
	name=$2
	src=$3
	if ((src in IGNORE) && ((name ~ IGNORE[src]) || (id ~ IGNORE[src])))
		next
	out(id, name, src)
}
'

echo '
</table>
</body>
</html>
'
}

