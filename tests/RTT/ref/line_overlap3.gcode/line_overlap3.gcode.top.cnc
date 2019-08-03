(Created by G-code exporter)
(600 dpi)
(Unit: inch)
(Board size: 0.50x0.50 inches)#100=0.002000  (safe Z)
#101=-0.000050  (cutting depth)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7 F1
G0 Z#100
(polygon 1)
G0 X0.196667 Y0.315000    (start point)
G1 Z#101
G1 X0.185000 Y0.303333
G1 X0.185000 Y0.295000
G1 X0.196667 Y0.283333
G1 X0.355000 Y0.283333
G1 X0.366667 Y0.295000
G1 X0.366667 Y0.303333
G1 X0.355000 Y0.315000
G1 X0.196667 Y0.315000
G0 Z#100
(polygon end, distance 0.40)
(end, total distance 10.14mm = 0.40in)
M5 M9 M2
