module part_0201(len=0.6, width=0.3, height=0.23, pad_len=0.15)
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

