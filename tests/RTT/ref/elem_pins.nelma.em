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
		"air-bottom"
	}
}
