//SCAD

module line_segment_r(length, width, thickness, x, y, a, bd, c1, c2) {
	translate([x,y,0]) rotate ([0,0,a]) union() {
		if (bd) {cube ([length, width,thickness],true);}
		if (c2) {translate([length/2.,0,0]) cylinder(h=thickness, r=width/2,center=true,$fn=30);}
		if (c1) { translate([-length/2.,0,0]) cylinder(h=thickness, r=width/2,center=true,$fn=30);}
	}
}

module line_segment(length, width, thickness, x, y, a) {
	translate([x,y,0]) rotate ([0,0,a]) {
		cube ([length, width,thickness],true);
	}
}

// START_OF_LAYER: topsilk
module layer_topsilk_body (offset) {
translate ([0, 0, offset]) union () {
	line_segment_r(2.540000,0.254000,0.037500,4.445000,-5.080000,90.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.037500,4.635500,-2.362200,180.000000,1,1,1);
	line_segment_r(0.508000,0.177800,0.037500,4.699000,-2.921000,180.000000,1,1,1);
	line_segment_r(1.016000,0.177800,0.037500,4.445000,-2.413000,90.000000,1,1,1);
	line_segment_r(0.508000,0.177800,0.037500,4.699000,-1.905000,180.000000,1,1,1);
	line_segment_r(0.287368,0.177800,0.037500,5.359401,-2.006600,-135.000000,1,1,1);
	line_segment_r(1.016000,0.177800,0.037500,5.461001,-2.413000,90.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.037500,5.448301,-2.921000,180.000000,1,1,1);
}
}


// END_OF_LAYER layer_topsilk

// START_OF_LAYER: bottomsilk
module layer_bottomsilk_body (offset) {
translate ([0, 0, offset]) union () {
}
}


// END_OF_LAYER layer_bottomsilk

module board_outline () {
	polygon([[0,0],[0,-12.700000],[12.700000,-12.700000],[12.700000,0]],
[[0,1,2,3]]);
}

module all_holes() {
	plating=0.017500;
	union () {
		for (i = layer_pdrill_list) {
			translate([i[1][0],i[1][1],0]) cylinder(r=i[0]+2*plating, h=1.770000, center=true, $fn=30);
		}
		for (i = layer_udrill_list) {
			translate([i[1][0],i[1][1],0]) cylinder(r=i[0], h=1.770000, center=true, $fn=30);
		}
	}
}

module board_body() {
	translate ([0, 0, -0.800000]) linear_extrude(height=1.600000) board_outline();}

/***************************************************/
/*                                                 */
/* Components                                      */
/*                                                 */
/***************************************************/
module all_components() {
}

/***************************************************/
/*                                                 */
/* Final board assembly                            */
/* Here is the complete board built from           */
/* pre-generated modules                           */
/*                                                 */
/***************************************************/
		color ([1, 1, 1])
			layer_topsilk_body(0.818750);

		color ([1, 1, 1])
			layer_bottomsilk_body(-0.818750);

		color ([0.44, 0.44, 0])
			difference() {
				board_body();
				all_holes();
			}

		all_components();
// END_OF_BOARD
