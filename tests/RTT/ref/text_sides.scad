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
	line_segment_r(0.762000,0.177800,0.037500,5.715000,-5.080000,90.000000,1,1,1);
	line_segment_r(0.310047,0.177800,0.037500,5.803900,-4.572000,-124.992020,1,1,1);
	line_segment_r(0.279400,0.177800,0.037500,6.032500,-4.445000,180.000000,1,1,1);
	line_segment_r(0.310047,0.177800,0.037500,6.261100,-4.572000,124.992020,1,1,1);
	line_segment_r(0.762000,0.177800,0.037500,6.350000,-5.080000,90.000000,1,1,1);
	line_segment_r(0.635000,0.177800,0.037500,6.032500,-4.953000,180.000000,1,1,1);
	line_segment_r(0.287368,0.177800,0.037500,6.756401,-4.546600,-135.000000,1,1,1);
	line_segment_r(1.016000,0.177800,0.037500,6.858001,-4.953000,90.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.037500,6.845301,-5.461000,180.000000,1,1,1);
}
}


// END_OF_LAYER layer_topsilk

// START_OF_LAYER: bottomsilk
module layer_bottomsilk_body (offset) {
translate ([0, 0, offset]) union () {
	line_segment_r(0.762000,0.177800,0.037500,5.715000,-6.985000,-90.000000,1,1,1);
	line_segment_r(0.310047,0.177800,0.037500,5.803900,-7.493000,124.992020,1,1,1);
	line_segment_r(0.279400,0.177800,0.037500,6.032500,-7.620000,180.000000,1,1,1);
	line_segment_r(0.310047,0.177800,0.037500,6.261100,-7.493000,-124.992020,1,1,1);
	line_segment_r(0.762000,0.177800,0.037500,6.350000,-6.985000,-90.000000,1,1,1);
	line_segment_r(0.635000,0.177800,0.037500,6.032500,-7.112000,180.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.037500,6.718301,-7.556500,135.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.037500,6.972301,-7.620000,180.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.037500,7.226301,-7.556500,-135.000000,1,1,1);
	line_segment_r(0.254000,0.177800,0.037500,7.289801,-7.366000,-90.000000,1,1,1);
	line_segment_r(0.898026,0.177800,0.037500,6.972301,-6.921500,135.000000,1,1,1);
	line_segment_r(0.635000,0.177800,0.037500,6.972301,-6.604000,180.000000,1,1,1);
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
