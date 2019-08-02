(Created by G-code exporter)
(600 dpi)
(Unit: inch)
(Board size: 0.50x0.50 inches)#100=0.002000  (safe Z)
#101=-0.000050  (cutting depth)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7 F1
G0 Z#100
(polygon 1)
G0 X0.188333 Y0.186667    (start point)
G1 Z#101
G1 X0.188333 Y0.111667
G1 X0.241667 Y0.111667
G1 X0.241667 Y0.186667
G1 X0.188333 Y0.186667
G0 Z#100
(polygon end, distance 0.26)
(polygon 2)
G0 X0.306667 Y0.186667    (start point)
G1 Z#101
G1 X0.306667 Y0.111667
G1 X0.360000 Y0.111667
G1 X0.360000 Y0.186667
G1 X0.306667 Y0.186667
G0 Z#100
(polygon end, distance 0.26)
(end, total distance 13.04mm = 0.51in)
M5 M9 M2
