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
module layer_00_body (offset) {
translate ([0, 0, offset]) union () {
	line_segment_r(1.199896,0.203200,0.035000,5.715000,-2.225040,180.000000,1,1,1);
	line_segment_r(1.199896,0.203200,0.035000,5.715000,-4.124960,180.000000,1,1,1);
	line_segment_r(0.508000,0.177800,0.035000,8.978900,-2.374900,180.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.035000,9.296400,-2.438400,135.000000,1,1,1);
	line_segment_r(0.254000,0.177800,0.035000,9.359900,-2.628900,90.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.035000,9.296400,-2.819400,-135.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.035000,9.042400,-2.882900,180.000000,1,1,1);
	line_segment_r(1.016000,0.177800,0.035000,8.851900,-2.882900,90.000000,1,1,1);
	line_segment_r(0.592425,0.177800,0.035000,9.207500,-3.136900,120.963753,1,1,1);
	line_segment_r(0.287368,0.177800,0.035000,9.766301,-2.476500,-135.000000,1,1,1);
	line_segment_r(1.016000,0.177800,0.035000,9.867901,-2.882900,90.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.035000,9.855201,-3.390900,180.000000,1,1,1);
}
}


// END_OF_LAYER layer_00

// START_OF_LAYER: bottomsilk
module layer_01_body (offset) {
translate ([0, 0, offset]) union () {
	line_segment_r(1.199896,0.203200,0.035000,5.715000,-9.839960,180.000000,1,1,1);
	line_segment_r(1.199896,0.203200,0.035000,5.715000,-7.940040,180.000000,1,1,1);
	line_segment_r(0.508000,0.177800,0.035000,8.978900,-9.690100,180.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.035000,9.296400,-9.626600,-135.000000,1,1,1);
	line_segment_r(0.254000,0.177800,0.035000,9.359900,-9.436100,-90.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.035000,9.296400,-9.245600,135.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.035000,9.042400,-9.182100,180.000000,1,1,1);
	line_segment_r(1.016000,0.177800,0.035000,8.851900,-9.182100,-90.000000,1,1,1);
	line_segment_r(0.592425,0.177800,0.035000,9.207500,-8.928100,-120.963753,1,1,1);
	line_segment_r(0.179605,0.177800,0.035000,9.728201,-9.626600,135.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.035000,9.982201,-9.690100,180.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.035000,10.236201,-9.626600,-135.000000,1,1,1);
	line_segment_r(0.254000,0.177800,0.035000,10.299701,-9.436100,-90.000000,1,1,1);
	line_segment_r(0.898026,0.177800,0.035000,9.982201,-8.991600,135.000000,1,1,1);
	line_segment_r(0.635000,0.177800,0.035000,9.982201,-8.674100,180.000000,1,1,1);
}
}


// END_OF_LAYER layer_01

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
		color ([0.44, 0.44, 0])
			difference() {
				board_body();
				all_holes();
			}

		all_components();
// END_OF_BOARD
