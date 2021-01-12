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
	types="board footprint netlist image misc"
	pends = 0
	disabled = 0
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
	flush_pends()
	next
}

($1 == "@@@@@@") {
	flush_pends()
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

# at the end of reading a plugin, write all pending data collected from
# the pup file if the plugin is not disabled
function flush_pends(    n)
{
	if (!disabled) {
		for(n = 0; n < pends; n++)
			add(PENDING1[n], PENDING2[n])
	}
	pends = 0
	disabled = 0
}

(mode == 1) && ($1 == "$fmt-feature-r") {
	$1=""
	PENDING1[pends] = $0
	PENDING2[pends] = "import"
	pends++
	next
}

(mode == 1) && ($1 == "$fmt-feature-w") {
	$1=""
	PENDING1[pends] = $0
	PENDING2[pends] = "export"
	pends++
	next
}

(mode == 1) && ($1 == "default") {
	if ($2 == "disable-all")
		disabled = 1
}

(mode != 2) { next }
'"$1"

}
