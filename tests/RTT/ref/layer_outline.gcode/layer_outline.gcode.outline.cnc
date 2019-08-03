(Created by G-code exporter)
(600 dpi)
(Unit: inch)
(Board size: 0.50x0.50 inches)#100=0.002000  (safe Z)
#101=-0.000050  (cutting depth)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7 F1
G0 Z#100
(polygon 1)
G0 X0.073333 Y0.410000    (start point)
G1 Z#101
G1 X0.065000 Y0.401667
G1 X0.065000 Y0.096667
G1 X0.073333 Y0.088333
G1 X0.378333 Y0.088333
G1 X0.386667 Y0.096667
G1 X0.386667 Y0.101667
G1 X0.078333 Y0.410000
G1 X0.073333 Y0.410000
G0 Z#100
(polygon end, distance 1.09)
(polygon 2)
G0 X0.086667 Y0.373333    (start point)
G1 Z#101
G1 X0.086667 Y0.110000
G1 X0.350000 Y0.110000
G1 X0.088333 Y0.373333
G1 X0.086667 Y0.373333
G0 Z#100
(polygon end, distance 0.90)
(end, total distance 50.57mm = 1.99in)
M5 M9 M2
