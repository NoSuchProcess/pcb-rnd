/* banner */
 */
/* **** Nets **** */


/* **** Objects **** */


/* **** Layers **** */

layer air-top {
	height = 40
	z-order = 1
	material = "air"
}
layer air-bottom {
	height = 40
	z-order = 1000
	material = "air"
}
layer top {
	height = 1
	z-order = 10
	material = "air"
	objects = {

	}
}
layer substrate-11 {
	height = 20
	z-order = 11
	material = "composite"
}
layer bottom {
	height = 1
	z-order = 12
	material = "air"
	objects = {

	}
}
layer substrate-13 {
	height = 20
	z-order = 13
	material = "composite"
}
layer group-1 {
	height = 1
	z-order = 14
	material = "air"
	objects = {

	}
}

/* **** Materials **** */

material copper {
	type = "metal"
	permittivity = 8.850000e-12
	conductivity = 0.0
	permeability = 0.0
}
material air {
	type = "dielectric"
	permittivity = 8.850000e-12
	conductivity = 0.0
	permeability = 0.0
}
material composite {
	type = "dielectric"
	permittivity = 3.540000e-11
	conductivity = 0.0
	permeability = 0.0
}

/* **** Space **** */

space pcb {
	step = { 2.540000e-04, 2.540000e-04, 1.000000e-04 }
	layers = {
		"air-top",
		"air-bottom",
		"top",
		"substrate-11",
		"bottom",
		"substrate-13",
		"group-1"
	}
}
