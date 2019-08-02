(Created by G-code exporter)
(600 dpi)
(Unit: inch)
(Board size: 0.50x0.50 inches)#100=0.002000  (safe Z)
#101=-0.000050  (cutting depth)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7 F1
G0 Z#100
(polygon 1)
G0 X0.050000 Y0.475000    (start point)
G1 Z#101
G1 X0.050000 Y0.048333
G1 X0.076667 Y0.048333
G1 X0.076667 Y0.475000
G1 X0.050000 Y0.475000
G0 Z#100
(polygon end, distance 0.91)
(polygon 2)
G0 X0.125000 Y0.475000    (start point)
G1 Z#101
G1 X0.125000 Y0.448333
G1 X0.476667 Y0.448333
G1 X0.476667 Y0.475000
G1 X0.125000 Y0.475000
G0 Z#100
(polygon end, distance 0.76)
(polygon 3)
G0 X0.125000 Y0.425000    (start point)
G1 Z#101
G1 X0.125000 Y0.048333
G1 X0.401667 Y0.048333
G1 X0.401667 Y0.425000
G1 X0.125000 Y0.425000
G0 Z#100
(polygon end, distance 1.31)
(polygon 4)
G0 X0.425000 Y0.075000    (start point)
G1 Z#101
G1 X0.425000 Y0.048333
G1 X0.451667 Y0.048333
G1 X0.451667 Y0.075000
G1 X0.425000 Y0.075000
G0 Z#100
(polygon end, distance 0.11)
(end, total distance 78.15mm = 3.08in)
M5 M9 M2
