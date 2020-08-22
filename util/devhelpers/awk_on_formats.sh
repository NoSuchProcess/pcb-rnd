# shell lib

# Read and parse all pups, make an array of file formats of
# FMTS[import|export, type] then read stdin and execute the awk
# script specified in $1 on it
awk_on_formats()
{
(for n in $ROOT/src_plugins/*/*.pup
do
	echo "@@@ $n"
	cat $n
done
echo "@@@@@@"
cat
) | awk '

BEGIN {
	osep = " <br> "
	types="netlist footprint board image misc"
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

function add(name, dir   ,type,lname)
{
	lname = tolower(name)
# DO NOT FORGET TO UDPATE types IN BEGIN ^^^
	if (name ~ "netlist")
		type = "netlist"
	else if (lname ~ "schematic")
		type = "netlist"
	else if (lname ~ "footprint")
		type = "footprint"
	else if (lname ~ "kicad.*module")
		type = "footprint"
	else if (lname ~ "board")
		type = "board"
	else if (lname ~ "render")
		type = "image"
	else if (lname ~ "pixmap")
		type = "image"
	else if (lname ~ "bitmap")
		type = "image"
	else if (lname ~ "graphic")
		type = "image"
	else
		type = "misc"
# DO NOT FORGET TO UDPATE types IN BEGIN ^^^

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
