# shell lib

awk_on_formats()
{
(for n in ../src_plugins/*/*.pup
do
	echo "@@@ $n"
	cat $n
done
echo "@@@@@@"
cat
) | awk '

BEGIN {
	osep = " <br> "
}

($1 == "@@@") {
	mode = 1
	plugin=$n
	sub(".*/", "", plugin)
	sub("\.pup", "", plugin)
	fmt = plugin
	sub("io_", "", fmt)
	sub("import_", "", fmt)
	sub("export_", "", fmt)
	next
}

($1 == "@@@@@@") {
	mode = 2
	next
}

function add(name, dir   ,type)
{
	if (name ~ "netlist")
		type = "netlist"
	else if (name ~ "schematic")
		type = "netlist"
	else if (name ~ "footprint")
		type = "footprint"
	else if (name ~ "board")
		type = "board"
	else if (name ~ "render")
		type = "image"
	else
		type = "misc"

	if (FMTS[dir, type] == "")
		FMTS[dir, type] = name
	else
		FMTS[dir, type] = FMTS[dir, type] osep name
}

(mode == 1) && ($1 == "$fmt-feature-r") {
	$1=""
	add($0, "import")
	next
}

(mode == 1) && ($1 == "$fmt-feature-w") {
	$1=""
	add($0, "export")
	next
}

(mode != 2) { next }
'"$1"

}
