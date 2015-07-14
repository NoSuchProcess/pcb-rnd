BEGIN {
	element_begin("100 mil connector", "CONN1", nx "*" ny    ,0,0, 0,mil(-100))

	for(x = 0; x < nx; x++)
		for(y = 0; y < ny; y++)
			element_pin(mil(x*100), mil(y*100))

	element_rectangle(mil(-50), mil(-50), mil(nx*100-50), mil(ny*100-50))

	element_line(mil(0), mil(-50),  mil(-50), mil(0))


	element_end()
	exit
}
