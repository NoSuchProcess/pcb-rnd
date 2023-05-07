module led(diameter)
{
    height = 1.65*diameter + 0.35;
    union () {
        color([1.0,0.1,0.1]) {
            translate ([0.0,0.0,height-diameter/2])
                sphere(r = diameter/2, $fn = 50);
            cylinder(r = diameter/2, h = height-diameter/2, $fn = 50);
            intersection () {
                translate ([-0.425,0,0.5])
                    cube ([diameter+0.9, diameter+0.9, 1.0], true);
                cylinder(r = (diameter+0.9)/2, h = 1.0, $fn = 50);
            }
        }
        color([0.8,0.8,0.8]) {
            translate ([-1.252,0,-1.2])
                cube ([0.5, 0.5, 2.5], true);
            translate ([1.252,0,-1.2])
                cube ([0.5, 0.5, 2.5], true);
        }
    }
}
