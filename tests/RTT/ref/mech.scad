// Round cap line
module pcb_line_rc(x1, y1, length, angle, width, thick) {
	translate([x1,y1,0]) {
		rotate([0,0,angle]) {
			translate([length/2, 0, 0])
				cube([length, width, thick], center=true);
			cylinder(r=width/2, h=thick, center=true, $fn=30);
			translate([length, 0, 0])
				cylinder(r=width/2, h=thick, center=true, $fn=30);
		}
	}
}
// Square cap line
module pcb_line_sc(x1, y1, length, angle, width, thick) {
	translate([x1,y1,0]) {
		rotate([0,0,angle]) {
			translate([length/2, 0, 0])
				cube([length + width, width, thick], center=true);
		}
	}
}
// filled rectangle
module pcb_fill_rect(x1, y1, x2, y2, angle, thick) {
	translate([(x1+x2)/2,(y1+y2)/2,0])
		rotate([0,0,angle])
			cube([x2-x1, y2-y1, thick], center=true);
}
// filled polygon
module pcb_fill_poly(coords, thick) {
	linear_extrude(height=thick)
		polygon(coords);
}
// filled circle
module pcb_fcirc(x1, y1, radius, thick) {
	translate([x1,y1,0])
		cylinder(r=radius, h=thick, center=true, $fn=30);
}
module pcb_outline() {
	polygon([
		[0.0000,12.7000],[12.7000,12.7000],[12.7000,0.0000],[0.0000,0.0000]
	]);
}
module layer_bottom_silk_pos_1() {
	color([0,0,0])
		translate([0,0,-0.833000]) {
		}
}

module layer_group_bottom_silk() {
	layer_bottom_silk_pos_1();
}

module layer_bottom_copper_pos_2() {
	color([1,0.4,0.2])
		translate([0,0,-0.811000]) {
			pcb_fill_poly([[3.0480,8.6360],[4.3180,8.6360],[4.3180,7.6200],[3.0480,7.6200]], 0.010000);
			pcb_fill_poly([[5.0800,8.6360],[6.3500,8.6360],[6.3500,7.6200],[5.0800,7.6200]], 0.010000);
			pcb_fill_poly([[3.1750,11.9380],[3.9370,11.9380],[3.9370,10.9220],[3.1750,10.9220]], 0.010000);
			pcb_fill_poly([[5.2070,11.9380],[5.9690,11.9380],[5.9690,10.9220],[5.2070,10.9220]], 0.010000);
			pcb_line_rc(3.5560, 9.6520, 0.5080, 90.000000, 0.5080, 0.010000);
			pcb_line_rc(5.5880, 9.6520, 0.5080, 90.000000, 0.5080, 0.010000);
			pcb_fill_poly([[3.4290,4.8260],[4.1910,4.8260],[4.1910,3.8100],[3.4290,3.8100]], 0.010000);
			pcb_fill_poly([[5.4610,4.8260],[6.2230,4.8260],[6.2230,3.8100],[5.4610,3.8100]], 0.010000);
			pcb_fcirc(3.5560, 9.9060, 0.1270, 0.010000);
			// line-approx arc 20.000000 .. 310.000000 by 10.000000
				pcb_line_rc(3.4516, 9.8680, 0.0194, -65.001333, 0.0000, 0.010000);
				pcb_line_rc(3.4598, 9.8504, 0.0194, -54.998045, 0.0000, 0.010000);
				pcb_line_rc(3.4709, 9.8346, 0.0194, -45.000000, 0.0000, 0.010000);
				pcb_line_rc(3.4846, 9.8209, 0.0194, -35.001955, 0.0000, 0.010000);
				pcb_line_rc(3.5004, 9.8098, 0.0194, -24.998667, 0.0000, 0.010000);
				pcb_line_rc(3.5180, 9.8016, 0.0194, -14.999059, 0.0000, 0.010000);
				pcb_line_rc(3.5367, 9.7966, 0.0194, -5.002155, 0.0000, 0.010000);
				pcb_line_rc(3.5560, 9.7949, 0.0194, 5.002413, 0.0000, 0.010000);
				pcb_line_rc(3.5753, 9.7966, 0.0194, 14.999059, 0.0000, 0.010000);
				pcb_line_rc(3.5940, 9.8016, 0.0194, 24.998667, 0.0000, 0.010000);
				pcb_line_rc(3.6116, 9.8098, 0.0194, 35.001955, 0.0000, 0.010000);
				pcb_line_rc(3.6274, 9.8209, 0.0194, 45.000000, 0.0000, 0.010000);
				pcb_line_rc(3.6411, 9.8346, 0.0194, 54.998045, 0.0000, 0.010000);
				pcb_line_rc(3.6522, 9.8504, 0.0194, 65.001333, 0.0000, 0.010000);
				pcb_line_rc(3.6604, 9.8680, 0.0194, 75.000941, 0.0000, 0.010000);
				pcb_line_rc(3.6654, 9.8867, 0.0194, 84.997587, 0.0000, 0.010000);
				pcb_line_rc(3.6671, 9.9060, 0.0194, 95.002155, 0.0000, 0.010000);
				pcb_line_rc(3.6654, 9.9253, 0.0194, 104.999059, 0.0000, 0.010000);
				pcb_line_rc(3.6604, 9.9440, 0.0194, 114.998667, 0.0000, 0.010000);
				pcb_line_rc(3.6522, 9.9616, 0.0194, 125.001955, 0.0000, 0.010000);
				pcb_line_rc(3.6411, 9.9774, 0.0194, 135.000000, 0.0000, 0.010000);
				pcb_line_rc(3.6274, 9.9911, 0.0194, 144.998045, 0.0000, 0.010000);
				pcb_line_rc(3.6116, 10.0022, 0.0194, 155.001333, 0.0000, 0.010000);
				pcb_line_rc(3.5940, 10.0104, 0.0194, 165.000941, 0.0000, 0.010000);
				pcb_line_rc(3.5753, 10.0154, 0.0194, 175.000534, 0.0000, 0.010000);
				pcb_line_rc(3.5560, 10.0171, 0.0194, -175.000792, 0.0000, 0.010000);
				pcb_line_rc(3.5367, 10.0154, 0.0194, -165.000941, 0.0000, 0.010000);
				pcb_line_rc(3.5180, 10.0104, 0.0194, -155.001333, 0.0000, 0.010000);
				pcb_line_rc(3.5004, 10.0022, 0.0194, -144.998045, 0.0000, 0.010000);
			pcb_fcirc(5.5880, 9.9060, 0.1270, 0.010000);
		}
}

module layer_group_bottom_copper() {
	layer_bottom_copper_pos_2();
}

module layer_top_copper_pos_3() {
	color([1,0.4,0.2])
		translate([0,0,0.811000]) {
			pcb_fill_poly([[3.0480,8.6360],[4.3180,8.6360],[4.3180,7.6200],[3.0480,7.6200]], 0.010000);
			pcb_fill_poly([[5.0800,8.6360],[6.3500,8.6360],[6.3500,7.6200],[5.0800,7.6200]], 0.010000);
			pcb_fill_poly([[3.1750,11.6840],[4.1910,11.6840],[4.1910,11.1760],[3.1750,11.1760]], 0.010000);
			pcb_fill_poly([[5.2070,11.6840],[6.2230,11.6840],[6.2230,11.1760],[5.2070,11.1760]], 0.010000);
			pcb_line_rc(3.3020, 9.9060, 0.5080, 0.000000, 0.5080, 0.010000);
			pcb_line_rc(5.3340, 9.9060, 0.5080, 0.000000, 0.5080, 0.010000);
			pcb_fill_poly([[3.4290,4.5720],[4.4450,4.5720],[4.4450,4.0640],[3.4290,4.0640]], 0.010000);
			pcb_fill_poly([[5.4610,4.5720],[6.4770,4.5720],[6.4770,4.0640],[5.4610,4.0640]], 0.010000);
			pcb_fcirc(3.5560, 9.9060, 0.1270, 0.010000);
			// line-approx arc 20.000000 .. 310.000000 by 10.000000
				pcb_line_rc(3.4516, 9.8680, 0.0194, -65.001333, 0.0000, 0.010000);
				pcb_line_rc(3.4598, 9.8504, 0.0194, -54.998045, 0.0000, 0.010000);
				pcb_line_rc(3.4709, 9.8346, 0.0194, -45.000000, 0.0000, 0.010000);
				pcb_line_rc(3.4846, 9.8209, 0.0194, -35.001955, 0.0000, 0.010000);
				pcb_line_rc(3.5004, 9.8098, 0.0194, -24.998667, 0.0000, 0.010000);
				pcb_line_rc(3.5180, 9.8016, 0.0194, -14.999059, 0.0000, 0.010000);
				pcb_line_rc(3.5367, 9.7966, 0.0194, -5.002155, 0.0000, 0.010000);
				pcb_line_rc(3.5560, 9.7949, 0.0194, 5.002413, 0.0000, 0.010000);
				pcb_line_rc(3.5753, 9.7966, 0.0194, 14.999059, 0.0000, 0.010000);
				pcb_line_rc(3.5940, 9.8016, 0.0194, 24.998667, 0.0000, 0.010000);
				pcb_line_rc(3.6116, 9.8098, 0.0194, 35.001955, 0.0000, 0.010000);
				pcb_line_rc(3.6274, 9.8209, 0.0194, 45.000000, 0.0000, 0.010000);
				pcb_line_rc(3.6411, 9.8346, 0.0194, 54.998045, 0.0000, 0.010000);
				pcb_line_rc(3.6522, 9.8504, 0.0194, 65.001333, 0.0000, 0.010000);
				pcb_line_rc(3.6604, 9.8680, 0.0194, 75.000941, 0.0000, 0.010000);
				pcb_line_rc(3.6654, 9.8867, 0.0194, 84.997587, 0.0000, 0.010000);
				pcb_line_rc(3.6671, 9.9060, 0.0194, 95.002155, 0.0000, 0.010000);
				pcb_line_rc(3.6654, 9.9253, 0.0194, 104.999059, 0.0000, 0.010000);
				pcb_line_rc(3.6604, 9.9440, 0.0194, 114.998667, 0.0000, 0.010000);
				pcb_line_rc(3.6522, 9.9616, 0.0194, 125.001955, 0.0000, 0.010000);
				pcb_line_rc(3.6411, 9.9774, 0.0194, 135.000000, 0.0000, 0.010000);
				pcb_line_rc(3.6274, 9.9911, 0.0194, 144.998045, 0.0000, 0.010000);
				pcb_line_rc(3.6116, 10.0022, 0.0194, 155.001333, 0.0000, 0.010000);
				pcb_line_rc(3.5940, 10.0104, 0.0194, 165.000941, 0.0000, 0.010000);
				pcb_line_rc(3.5753, 10.0154, 0.0194, 175.000534, 0.0000, 0.010000);
				pcb_line_rc(3.5560, 10.0171, 0.0194, -175.000792, 0.0000, 0.010000);
				pcb_line_rc(3.5367, 10.0154, 0.0194, -165.000941, 0.0000, 0.010000);
				pcb_line_rc(3.5180, 10.0104, 0.0194, -155.001333, 0.0000, 0.010000);
				pcb_line_rc(3.5004, 10.0022, 0.0194, -144.998045, 0.0000, 0.010000);
			pcb_fcirc(5.5880, 9.9060, 0.1270, 0.010000);
		}
}

module layer_top_copper_pos_4() {
	color([1,0.4,0.2])
		translate([0,0,0.811000]) {
		}
}

module layer_group_top_copper() {
	layer_top_copper_pos_3();
	layer_top_copper_pos_4();
}

module layer_top_mask_pos_5() {
	color([0,0.7,0,0.5])
		translate([0,0,0.822000]) {
			pcb_fill_rect(0.0000, 0.0000, 12.7000, 12.7000, 0.000000, 0.010000);
		}
}

module layer_top_mask_neg_6() {
	color([0,0.7,0,0.5])
		translate([0,0,0.812000]) {
		}
}

module layer_top_mask_pos_7() {
	color([0,0.7,0,0.5])
		translate([0,0,0.822000]) {
		}
}

module layer_group_top_mask() {
	union() {
	difference() {
	layer_top_mask_pos_5();
	layer_top_mask_neg_6();
}
	layer_top_mask_pos_7();
}
}

module layer_bottom_mask_pos_8() {
	color([0,0.7,0,0.5])
		translate([0,0,-0.822000]) {
			pcb_fill_rect(0.0000, 0.0000, 12.7000, 12.7000, 0.000000, 0.010000);
		}
}

module layer_bottom_mask_neg_9() {
	color([0,0.7,0,0.5])
		translate([0,0,-0.832000]) {
		}
}

module layer_bottom_mask_pos_10() {
	color([0,0.7,0,0.5])
		translate([0,0,-0.822000]) {
		}
}

module layer_group_bottom_mask() {
	union() {
	difference() {
	layer_bottom_mask_pos_8();
	layer_bottom_mask_neg_9();
}
	layer_bottom_mask_pos_10();
}
}

module layer_top_silk_pos_11() {
	color([0,0,0])
		translate([0,0,0.833000]) {
		}
}

module layer_top_silk_pos_12() {
	color([0,0,0])
		translate([0,0,0.833000]) {
		}
}

module layer_group_top_silk() {
	layer_top_silk_pos_11();
	layer_top_silk_pos_12();
}

module pcb_drill() {
	translate([3.5560,9.9060,0])
		cylinder(r=0.1270, h=4, center=true, $fn=30);
	translate([5.5880,9.9060,0])
		cylinder(r=0.1270, h=4, center=true, $fn=30);
}
module pcb_board_main() {
	translate ([0, 0, -0.8])
		linear_extrude(height=1.6)
			pcb_outline();
	layer_group_bottom_silk();
	layer_group_bottom_copper();
	layer_group_top_copper();
	layer_group_top_mask();
	layer_group_bottom_mask();
	layer_group_top_silk();
}

module pcb_board() {
	intersection() {
		translate ([0, 0, -4])
			linear_extrude(height=8)
				pcb_outline();
		union() {
			difference() {
				pcb_board_main();
				pcb_drill();
			}
		}
	}
}

pcb_board();
