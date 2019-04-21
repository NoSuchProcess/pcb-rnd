module part_0402(len=1.0, width=0.5, height=0.35, pad_len=0.25)
{
	union() {
		translate([0,0,height/2]) {
			// body
			color([0.1,0.1,0.1])
				cube([len-2*pad_len,width,height], center=true);
			// terminals
			color([0.8,0.8,0.8]) {
				translate([+len/2-pad_len/2, 0, 0])
					cube([pad_len, width, height], center=true);
			}
			color([0.8,0.8,0.8]) {
				translate([-len/2+pad_len/2, 0, 0])
					cube([pad_len, width, height], center=true);
			}
		}
	}
}

