(Created by G-code exporter)
(600 dpi)
(Unit: inch)
(Board size: 0.50x0.50 inches)#100=0.002000  (safe Z)
#101=-0.000050  (cutting depth)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7 F1
G0 Z#100
(polygon 1)
G0 X0.098333 Y0.310000    (start point)
G1 Z#101
G1 X0.090000 Y0.301667
G1 X0.090000 Y0.296667
G1 X0.098333 Y0.288333
G1 X0.500000 Y0.288333
G1 X0.500000 Y0.310000
G1 X0.098333 Y0.310000
G0 Z#100
(polygon end, distance 0.85)
(end, total distance 21.68mm = 0.85in)
M5 M9 M2
