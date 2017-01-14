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
	line_segment_r(2.540000,0.254000,0.037500,0.635000,-3.175000,90.000000,1,1,1);
	line_segment_r(10.160000,0.254000,0.037500,5.715000,-4.445000,0.000000,1,1,1);
	line_segment_r(2.540000,0.254000,0.037500,10.795000,-3.175000,-90.000000,1,1,1);
	line_segment_r(3.810000,0.254000,0.037500,2.540000,-1.905000,180.000000,1,1,1);
	line_segment_r(3.810000,0.254000,0.037500,8.890000,-1.905000,180.000000,1,1,1);
	line_segment_r(0.110792,0.254000,0.037500,4.447417,-1.960343,92.499641,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,4.457064,-2.070610,97.500259,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,4.476285,-2.179616,102.499977,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,4.504933,-2.286532,107.499977,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,4.542790,-2.390545,112.500046,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,4.589569,-2.490862,117.499908,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,4.644913,-2.586720,122.499672,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,4.708401,-2.677391,127.500198,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,4.779550,-2.762182,132.500183,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,4.857818,-2.840451,137.499817,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,4.942609,-2.911599,142.500107,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.033280,-2.975087,147.500046,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,5.129138,-3.030431,152.500092,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.229455,-3.077209,157.499954,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.333467,-3.115067,162.500031,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.440383,-3.143715,167.500031,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.549390,-3.162936,172.499741,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,5.659657,-3.172583,177.500366,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,5.770343,-3.172583,-177.500366,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.880610,-3.162936,-172.499741,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.989616,-3.143715,-167.500031,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.096532,-3.115067,-162.500031,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.200545,-3.077209,-157.499954,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,6.300862,-3.030431,-152.500092,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.396720,-2.975087,-147.500046,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.487391,-2.911599,-142.500107,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.572183,-2.840451,-137.499817,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.650451,-2.762182,-132.500183,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.721599,-2.677391,-127.499794,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.785087,-2.586721,-122.500381,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.840431,-2.490863,-117.499664,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.887209,-2.390545,-112.500046,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.925067,-2.286532,-107.499825,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.953715,-2.179615,-102.499977,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.972936,-2.070610,-97.500320,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,6.982583,-1.960343,-92.499641,1,1,1);
	line_segment_r(2.540000,0.254000,0.037500,0.635000,-8.255000,90.000000,1,1,1);
	line_segment_r(10.160000,0.254000,0.037500,5.715000,-9.525000,0.000000,1,1,1);
	line_segment_r(2.540000,0.254000,0.037500,10.795000,-8.255000,-90.000000,1,1,1);
	line_segment_r(3.810000,0.254000,0.037500,2.540000,-6.985000,180.000000,1,1,1);
	line_segment_r(3.810000,0.254000,0.037500,8.890000,-6.985000,180.000000,1,1,1);
	line_segment_r(0.110792,0.254000,0.037500,4.447417,-7.040343,92.499641,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,4.457064,-7.150610,97.500259,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,4.476285,-7.259616,102.499977,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,4.504933,-7.366532,107.499977,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,4.542790,-7.470545,112.500046,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,4.589569,-7.570862,117.499908,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,4.644913,-7.666720,122.499672,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,4.708401,-7.757391,127.500198,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,4.779550,-7.842183,132.500183,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,4.857818,-7.920451,137.499817,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,4.942609,-7.991600,142.500107,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.033280,-8.055087,147.500046,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,5.129138,-8.110431,152.500092,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.229455,-8.157209,157.499954,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.333467,-8.195067,162.500031,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.440383,-8.223715,167.500031,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.549390,-8.242936,172.499741,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,5.659657,-8.252583,177.500366,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,5.770343,-8.252583,-177.500366,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.880610,-8.242936,-172.499741,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.989616,-8.223715,-167.500031,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.096532,-8.195067,-162.500031,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.200545,-8.157209,-157.499954,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,6.300862,-8.110431,-152.500092,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.396720,-8.055087,-147.500046,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.487391,-7.991600,-142.500107,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.572183,-7.920451,-137.499817,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.650451,-7.842183,-132.500183,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.721599,-7.757391,-127.499794,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.785087,-7.666721,-122.500381,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.840431,-7.570862,-117.499664,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.887209,-7.470545,-112.500046,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.925067,-7.366532,-107.499825,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.953715,-7.259615,-102.499977,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.972936,-7.150609,-97.500320,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,6.982583,-7.040343,-92.499641,1,1,1);
	line_segment_r(0.889000,0.177800,0.037500,1.905000,-1.079500,90.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.037500,1.968500,-1.587500,135.000000,1,1,1);
	line_segment_r(0.254000,0.177800,0.037500,2.159000,-1.651000,180.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.037500,2.349500,-1.587500,-135.000000,1,1,1);
	line_segment_r(0.889000,0.177800,0.037500,2.413000,-1.079500,90.000000,1,1,1);
	line_segment_r(0.287368,0.177800,0.037500,2.819401,-0.736600,-135.000000,1,1,1);
	line_segment_r(1.016000,0.177800,0.037500,2.921001,-1.143000,90.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.037500,2.908301,-1.651000,180.000000,1,1,1);
	line_segment_r(0.889000,0.177800,0.037500,1.905000,-6.159500,90.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.037500,1.968500,-6.667500,135.000000,1,1,1);
	line_segment_r(0.254000,0.177800,0.037500,2.159000,-6.731000,180.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.037500,2.349500,-6.667500,-135.000000,1,1,1);
	line_segment_r(0.889000,0.177800,0.037500,2.413000,-6.159500,90.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.037500,2.781301,-5.778500,-135.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.037500,3.035301,-5.715000,180.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.037500,3.289301,-5.778500,135.000000,1,1,1);
	line_segment_r(0.254000,0.177800,0.037500,3.352801,-5.969000,90.000000,1,1,1);
	line_segment_r(0.898026,0.177800,0.037500,3.035301,-6.413500,-135.000000,1,1,1);
	line_segment_r(0.635000,0.177800,0.037500,3.035301,-6.731000,180.000000,1,1,1);
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
