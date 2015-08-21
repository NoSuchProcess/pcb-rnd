function parri(A     ,s,i)
{
	for(i in A)
		s = s " " i
	return s
}

BEGIN {

	PT["50"] = "26mil"
	set_arg(P, "?pitch", "50")

	proc_args(P, "pins,size,pitch,cpad_size,pad_thickness", "pins")

	pitch = P["pitch"]
	sub("[.]0*$", "", pitch)

	if (!(args ~ "pad_thickness=")) {
		if (!(pitch in PT))
			error("Unkown pitch (" pitch "), should be one of:" parri(PT))
		pt = PT[pitch]
	}
	else
		pt = rev_mil(DEFAULT["pad_thickness"])

	if (P["size"] == "")
		P["size"] = int(P["pins"] * (pitch/4) + 140)

	split(P["size"], S, "x")
	if (S[2] == "")
		S[2] = S[1]
	if (S[1] != S[2])
		error("need n*n size")

	pins = int(P["pins"])
	if (pins / 4 != int(pins / 4))
		error("number of pins must be divisible by 4")

	pins=pins/4

	S[1] -= 60
	S[2] -= 60
	args = args ",nx=" pins ",ny=" pins ",x_spacing=" S[1] "mil,y_spacing=" S[2] "mil,pad_spacing=" pitch "mil,pad_thickness=" pt "mil"

	args = args ",width=" S[1]-150 " mil,height=" S[2]-150 " mil"


	if (P["cpad_size"] != "")
		args = args ",cpad_width=" P["cpad_size"] "mil,cpad_height=" P["cpad_size"] "mil"

	args = args ",int_bloat=47mil,ext_bloat=47mil"
	args = args ",?bodysilk=plcc,?silkmark=circle,pinoffs=" int(pins/2+0.5)


}
