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

// START_OF_LAYER: plated-drill
layer_pdrill_list=[
];


// END_OF_LAYER layer_pdrill

// START_OF_LAYER: topsilk
module layer_topsilk_body (offset) {
translate ([0, 0, offset]) union () {
	translate ([0, 0, -0.018750]) linear_extrude(height=0.037500) polygon ([		[0.635000, -0.635000], 
		[4.445000, -0.635000], 
		[4.445000, -4.445000], 
		[0.635000, -4.445000]
	],[[
		0, 1, 2, 3]]);
	translate ([0, 0, -0.018750]) linear_extrude(height=0.037500) polygon ([		[5.080000, -0.635000], 
		[12.065000, -0.635000], 
		[12.065000, -12.065000], 
		[5.080000, -12.065000]
	],[[
		0, 1, 2, 3]]);
	translate ([0, 0, -0.018750]) linear_extrude(height=0.037500) polygon ([		[5.715000, -1.270000], 
		[11.430000, -1.270000], 
		[11.430000, -11.430000], 
		[5.715000, -11.430000]
	],[[
		0, 1, 2, 3]]);
	line_segment_r(3.175000,2.032000,0.037500,2.540000,-4.762500,-90.000000,1,1,1);
	line_segment_r(0.166190,0.508000,0.037500,0.083016,-1.901376,-177.500046,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,0.248416,-1.886905,-172.500046,1,0,1);
	line_segment_r(0.166189,0.508000,0.037500,0.411925,-1.858074,-167.499985,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,0.572299,-1.815102,-162.499741,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,0.728318,-1.758315,-157.500107,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,0.878794,-1.688147,-152.499954,1,0,1);
	line_segment_r(0.166189,0.508000,0.037500,1.022581,-1.605132,-147.500198,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,1.158587,-1.509900,-142.499939,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,1.285774,-1.403177,-137.500061,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,1.403176,-1.285775,-132.499939,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,1.509899,-1.158587,-127.499855,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,1.605131,-1.022581,-122.500092,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,1.688147,-0.878794,-117.500053,1,0,1);
	line_segment_r(0.166189,0.508000,0.037500,1.758315,-0.728319,-112.500023,1,0,1);
	line_segment_r(0.166191,0.508000,0.037500,1.815102,-0.572299,-107.500153,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,1.858074,-0.411925,-102.499939,1,0,1);
	line_segment_r(0.166190,0.508000,0.037500,1.886905,-0.248415,-97.499962,1,0,1);
	line_segment_r(0.166189,0.508000,0.037500,1.901376,-0.083016,-92.499977,1,1,1);
	line_segment_r(2.540000,0.508000,0.037500,2.540000,-4.445000,-90.000000,1,1,1);
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
