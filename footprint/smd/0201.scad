module part_0201(len=0.6, width=0.3, height=0.23, pad_len=0.15, fillet=0)
{
    module fillet() {
        fillet_height = height/3;
        fillet_width = pad_len/3;        
        overall_width = fillet_width + width;
        overall_length = pad_len + fillet_width;

        translate([-pad_len/2,0,-height/2]) {
            fillet_points = [
                [0,overall_width/2,0], // 0
                [overall_length,overall_width/2,0], // 1
                [overall_length,-overall_width/2,0], // 2
                [0,-overall_width/2,0], // 3
                [0,width/2+(overall_width-width)/5,fillet_height/3], // 4
                [pad_len+fillet_width/2,width/2+(overall_width-width)/5,fillet_height/3], // 5
                [pad_len+fillet_width/2,-width/2-(overall_width-width)/5,fillet_height/3], // 6
                [0,-width/2-(overall_width-width)/5,fillet_height/3], // 7
                [0,width/2+(overall_width-width)/11,2*fillet_height/3], // 8
                [pad_len+fillet_width/6,width/2+(overall_width-width)/11,2*fillet_height/3], // 9
                [pad_len+fillet_width/6,-width/2-(overall_width-width)/11,2*fillet_height/3], // 10
                [0,-width/2-(overall_width-width)/11,2*fillet_height/3], // 11
                [0,width/2,fillet_height], // 12
                [pad_len,width/2,fillet_height], // 13
                [pad_len,-width/2,fillet_height], // 14
                [0,-width/2,fillet_height]]; // 15
    
            fillet_faces = [
                [0,4,8,12,15,11,7,3], // 0
                [0,1,5,4], // 1
                [1,2,6,5], // 2
                [7,6,2,3], // 3
                [4,5,9,8], // 4
                [5,6,10,9],// 5
                [10,6,7,11],// 6
                [8,9,13,12],// 7
                [9,10,14,13],// 8
                [15,14,10,11],// 9
                [12,13,14,15],// 10
                [3,2,1,0]];// 11

            polyhedron(fillet_points, fillet_faces);        
        }
    }

	union() {
		translate([0,0,height/2]) {
			// body
			color([0.1,0.1,0.1])
				cube([len-2*pad_len,width,height], center=true);
			// terminals
			color([0.8,0.8,0.8]) {
				translate([+len/2-pad_len/2, 0, 0]) {
					cube([pad_len, width, height], center=true);
                    if (fillet)
                        fillet(); 
                    }
				translate([-len/2+pad_len/2, 0, 0]) {
                    cube([pad_len, width, height], center=true);
                    if (fillet)
                        rotate([0,0,180])
                            fillet(); 
                    }
			}
		}
	}
}
