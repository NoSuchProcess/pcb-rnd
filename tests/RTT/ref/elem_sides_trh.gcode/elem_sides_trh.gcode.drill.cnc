(Created by G-code exporter)
(drill file: 4 drills)
(Unit: inch)
(Board size: 0.50x0.50 inches)#100=0.002000  (safe Z)
#101=-0.002000  (drill depth)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7 F1
G81 X0.100000 Y0.150000 Z#101 R#100
G81 X0.100000 Y0.350000 Z#101 R#100
G81 X0.400000 Y0.350000 Z#101 R#100
G81 X0.400000 Y0.150000 Z#101 R#100
M5 M9 M2
(end, total distance 17.78mm = 0.70in)
