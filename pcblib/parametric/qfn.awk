function parri(A     ,s,i)
{
	for(i in A)
		s = s " " i
	return s
}

BEGIN {
	help_auto()
	PT["0.65"] = "0.35mm"
	PT["0.5"] = "0.3mm"
	PT["0.4"] = "0.2mm"

	proc_args(P, "pins,size,pitch,cpad_size,pad_thickness", "pins,size,pitch")

	pitch = P["pitch"]
	sub("0*$", "", pitch)

	if (!(args ~ "pad_thickness=")) {
		if (!(pitch in PT))
			error("Unknown pitch, should be one of:" parri(PT))
		pt = PT[pitch]
	}
	else
		pt = rev_mm(DEFAULT["pad_thickness"]) "mm"

	split(P["size"], S, "x")
	if (S[2] == "")
		S[2] = S[1]
	if (S[1] != S[2])
		error("need n*n size")

	pins = int(P["pins"])
	if (pins / 4 != int(pins / 4))
		error("number of pins must be divisible by 4")

	pins=pins/4


	args = args ",nx=" pins ",ny=" pins ",x_spacing=" S[1] "mm,y_spacing=" S[2] "mm,pad_spacing=" pitch "mm,pad_thickness=" pt

	if (P["cpad_size"] != "")
		args = args ",cpad_width=" P["cpad_size"] "mm,cpad_height=" P["cpad_size"] "mm"

#	qf_globals("pins,size")

}
