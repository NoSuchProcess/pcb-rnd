BEGIN {
	q="\""
}

function parent(path)
{
	sub("/[^/]*$", "", path)
	return path
}

function children(DST, path)
{
	return split(CHILDREN[path], DST, "[|]")
}

function sy_is_recursive(path,  dp)
{
	dp = DATA[path]
	gsub("/[0-9]::", "/", path)
	if (path ~ dp)
		rturn 1
	return 0
}

function sy_href(path)
{
	return "#" path
}

(($1 == "open") || ($1 == "data")) {
	path=$3
	gsub("[0-9]+::", "", path)
	TYPE[path] = $2
	p = parent(path)
	if (CHILDREN[p] == "")
		CHILDREN[p] = path
	else
		CHILDREN[p] = CHILDREN[p] "|" path
	data=$4
	gsub("\\\\057", "/", data)
	DATA[path] = data
	
	name=$3
	sub("^.*/", "", name)
	sub(".*::", "", name)
	NAME[path] = name
}

function qstrip(s)
{
	gsub("[\\\\]+164", " ", s)
	gsub("[\\\\]n", " ", s)
	return s
}

function qstripnl(s)
{
	gsub("[\\\\]+164", " ", s)
	gsub("[\\\\]n", "\n", s)
	return s
}

