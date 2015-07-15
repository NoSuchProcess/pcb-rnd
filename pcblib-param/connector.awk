BEGIN {
	proc_args(P, "nx,ny", "nx,ny")


	element_begin("100 mil connector", "CONN1", P["nx"] "*" P["ny"]    ,0,0, 0,mil(-100))

	for(x = 0; x < P["nx"]; x++)
		for(y = 0; y < P["ny"]; y++)
			element_pin(mil(x*100), mil(y*100))

	element_rectangle(mil(-50), mil(-50), mil(P["nx"]*100-50), mil(P["ny"]*100-50))

	element_line(mil(0), mil(-50),  mil(-50), mil(0))


	element_end()
}
