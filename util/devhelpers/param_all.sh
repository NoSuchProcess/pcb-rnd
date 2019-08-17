#!/bin/sh

# Generate and render parametric footprints with variations of parameters
# for testing

fp="$1"
shift 1
args="$@"

$fp --help | awk -F "[: \t]+" -v fp=$fp -v "args=$args" '
BEGIN { order = 0 }

/#@@param/ { PARAMS[$2]++ }

/#@@param/ { ORDER[order++] = $2 }
/#@@dim/ { TYPE[$2] = "dim" }

/#@@enum/ {
	TYPE[$2] = "enum"
	if (ENUM[$2] == "")
		ENUM[$2] = $3
	else
		ENUM[$2] = ENUM[$2] "," $3
}

function build(param, value,     PV,p,s,cmd,fn)
{
	for(p in ARG)
		PV[p] = ARG[p]

	if (param != "")
		PV[param] = value

	for(o = 0; o < order; o++) {
		p = ORDER[o]
		if ((p == "") || !(p in PV))
			continue
		if (s == "")
			s = p "=" PV[p]
		else
			s = s "," p "=" PV[p]
	}


	fn = s
	gsub("[^A-Za-z0-9-]", "_", fn)
	fn = fp "_" fn  ".lht"
	system(fp " " q s q ">" fn)

	close(cmd)
	cmd="pcb-rnd --hid batch"
	print "LoadFrom(Layout, " fn ")" | cmd
	print "autocrop" | cmd
	print "export(png, --dpi, 1200)" | cmd
	close(cmd)
}

function permute(param   ,n,v,E)
{
	if (TYPE[param] == "enum") {
		v = split(ENUM[param], E, ",")
		for(n = 1; n <= v; n++) {
			build(param, E[n])
		}
	}
	if (TYPE[param] == "dim") {
		if (param in ARG) {
			build(param, ARG[param])
			build(param, ARG[param]*2)
			build(param, ARG[param]*4)
		}
		else {
			if ((param ~ "clearance") || (param ~ "mask") || (param ~ "paste")) {
				# skip
			}
			else if ((param ~ "drill") || (param ~ "thickness")) {
				build(param, "0.2mm")
				build(param, "0.4mm")
				build(param, "0.8mm")
			}
			else {
				build(param, "2mm")
				build(param, "4mm")
				build(param, "8mm")
			}
		}
	}
}

END {
	v = split(args, A, ",[ \t]*")
	for(n = 1; n <= v; n++) {
		split(A[n], B, "[=]")
		ARG[B[1]] = B[2]
	}
	for(p in PARAMS)
		permute(p)
}

'





