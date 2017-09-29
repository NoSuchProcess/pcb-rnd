module part_1206()
{
	translate([0,0,0.3]) {
		// body
		color([0.1,0.1,0.1])
			cube([3.2-2*0.2,1.6,0.6], center=true);
		// terminals
		color([0.8,0.8,0.8]) {
			translate([+1.5, 0, 0])
				cube([0.2, 1.6, 0.6], center=true);
			translate([-1.5, 0, 0])
				cube([0.2, 1.6, 0.6], center=true);
		}
	}
}
