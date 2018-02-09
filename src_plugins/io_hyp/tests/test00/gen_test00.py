#!/usr/bin/python
"""
  Generate sample hyperlynx file 
  Copyright 2017 Koen De Vleeschauwer.
 
  This file is part of pcb-rnd.
 
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

from math import pi, sin, cos

print "{VERSION=2.0}"
print "{UNITS=METRIC LENGTH}"
print "{BOARD"
print "  (PERIMETER_SEGMENT X1=1.0 Y1=0.0 X2=19.0 Y2=0.0)"
print "  (PERIMETER_SEGMENT X1=20.0 Y1=1.0 X2=20.0 Y2=9.0)"
print "  (PERIMETER_SEGMENT X1=19.0 Y1=10.0 X2=1.0 Y2=10.0)"
print "  (PERIMETER_SEGMENT X1=0.0 Y1=9.0 X2=0.0 Y2=1.0)"
print "  (PERIMETER_ARC X1=0.0 Y1=1.0 X2=1.0 Y2=0.0 XC=1.0 YC=1.0 R=1.0)"
print "  (PERIMETER_ARC X1=1.0 Y1=10.0 X2=0.0 Y2=9.0 XC=1.0 YC=9.0 R=1.0)"
print "  (PERIMETER_ARC X1=20.0 Y1=9.0 X2=19.0 Y2=10.0 XC=19.0 YC=9.0 R=1.0)"
print "  (PERIMETER_ARC X1=19.0 Y1=0.0 X2=20.0 Y2=1.0 XC=19.0 YC=1.0 R=1.0)"
print ""
print "  (PERIMETER_SEGMENT X1=2.0 Y1=1.0 X2=3.0 Y2=1.0)"
print "  (PERIMETER_SEGMENT X1=3.0 Y1=9.0 X2=2.0 Y2=9.0)"
print "  (PERIMETER_ARC X1=2.0 Y1=9.0 X2=1.0 Y2=8.0 XC=2.0 YC=8.0 R=1.0)"
print "  (PERIMETER_SEGMENT X1=1.0 Y1=8.0 X2=1.0 Y2=2.0)"
print "  (PERIMETER_ARC X1=1.0 Y1=2.0 X2=2.0 Y2=1.0 XC=2.0 YC=2.0 R=1.0)"
print "  (PERIMETER_SEGMENT X1=4.0 Y1=2.0 X2=4.0 Y2=8.0)"
print "  (PERIMETER_ARC X1=3.0 Y1=1.0 X2=4.0 Y2=2.0 XC=3.0 YC=2.0 R=1.0)"
print "  (PERIMETER_ARC X1=4.0 Y1=8.0 X2=3.0 Y2=9.0 XC=3.0 YC=8.0 R=1.0)"

print "  (PERIMETER_ARC X1=5.5 Y1=5.0 X2=5.5 Y2=5.0 XC=5.0 YC=5.0 R=0.5)"

print "  (PERIMETER_ARC X1=5.5 Y1=6.0 X2=4.5 Y2=6.0 XC=5.0 YC=6.0 R=0.5)"
print "  (PERIMETER_SEGMENT X1=4.5 Y1=6.0 X2=5.5 Y2=6.0)"

print "  (PERIMETER_ARC X1=4.5 Y1=7.5 X2=5.5 Y2=7.5 XC=5.0 YC=7.5 R=0.5)"
print "  (PERIMETER_SEGMENT X1=4.5 Y1=7.5 X2=5.5 Y2=7.5)"

print "  (PERIMETER_ARC X1=5.5 Y1=8.5 X2=5.0 Y2=8.0 XC=5.0 YC=8.5 R=0.5)"
print "  (PERIMETER_SEGMENT X1=5.0 Y1=8.0 X2=5.5 Y2=8.5)"

print "}"

print "{PLANE_SEP=0.05}"

print "{STACKUP"
print "  (SIGNAL T=0.000700 L=component)"
print "  (DIELECTRIC T=0.002000 C=4.000000)"
print "  (PLANE  T=0.000700 L=solder PS=0.508)"
print "}"
print "{DEVICES"
print "  (? REF=U1 NAME=BC548 L=component)"
print "}"

#
# PADSTACK. MDEF is shorthand for "all copper layers"
#

print "{PADSTACK=roundpad, 0.2"
print "  (MDEF, 0, 0.5, 0.5, 0)"
print "}"
print "{PADSTACK=squarepad, 0.2"
print "  (MDEF, 1, 0.5, 0.5, 0)"
print "}"
print "{PADSTACK=oblongpad, 0.2"
print "  (MDEF, 2, 0.75, 0.5, 0)"
print "}"
print "{PADSTACK=nodrill,"
print "  (MDEF, 0, 0.5, 0.5, 0)"
print "}"
print "{PADSTACK=0802pad,"
print "  (component, 1, 0.2, 0.12, 0)"
print "}"

#
# SEG line segments
#

x0 = 6
y0 = 3
r1 = 0.5
r2 = 2
max = 12

print "{NET=segtst"

for x in range (0, max):
  alpha = x * 2 * pi / max
  x1 = x0 + r1 * cos (alpha)
  y1 = y0 + r1 * sin (alpha)
  r = r1 + (r2 - r1) * (x + 1) / max
  x2 = x0 + r * cos (alpha)
  y2 = y0 + r * sin (alpha)
  w = (x + 1) * 0.1 / max
  print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=%f L=component)" % ( x1, y1, x2, y2, w)
  
print "}"

#
# ARC arc segments
#

max = 8
x0 = 9
y0 = 1
r = 0.1
w = r / 4

print "{NET=arctst"

for a in range (0, max):
  for b in range (0, max):
    alpha = 2 * pi * a / max
    beta = 2 * pi * b / max
    xc = x0 + 4  * r * a 
    yc = y0 + 4  * r * b
    x1 = xc + r * cos (alpha)
    y1 = yc + r * sin (alpha)
    x2 = xc + r * cos (beta)
    y2 = yc + r * sin (beta)
    print "  (ARC X1=%f Y1=%f X2=%f Y2=%f XC=%f YC=%f R=%f W=%f L=component)" % ( x1, y1, x2, y2, xc, yc, r, w)
  
print "}"

#
# VIA
#

print "{NET=viatst"
print "  (VIA X=13 Y=1 P=roundpad)"
print "  (VIA X=13 Y=2 P=squarepad)"
print "  (VIA X=13 Y=3 P=oblongpad)"
print "  (VIA X=13 Y=4 P=nodrill)"
print "  (SEG X1=13 Y1=1 X2=13 Y2=2 W=0.1 L=component)"
print "  (SEG X1=13 Y1=2 X2=13 Y2=3 W=0.1 L=component)"
print "  (SEG X1=13 Y1=3 X2=13 Y2=4 W=0.1 L=component)"
print "}"

#
# old-style VIA, compatibility only.
#

print "{NET=oldviatst"
print "  (VIA X=14 Y=1 D=0.2 L1=component S1=ROUND S1X=0.5 S1Y=0.5)"
print "  (VIA X=14 Y=2 D=0.2 L1=component L2=solder S1=RECT S1X=0.5 S1Y=0.5)"
print "  (VIA X=14 Y=3 D=0.2 L1=component L2=solder S1=OBLONG S1X=0.75 S1Y=0.5)"
print "  (VIA X=14 Y=4 D=0.0 L1=component L2=solder S1=ROUND S1X=0.5 S1Y=0.5)"
print "  (SEG X1=14 Y1=1 X2=14 Y2=2 W=0.1 L=component)"
print "  (SEG X1=14 Y1=2 X2=14 Y2=3 W=0.1 L=component)"
print "  (SEG X1=14 Y1=3 X2=14 Y2=4 W=0.1 L=component)"
print "}"

print "{NET=pintst"
# a through-hole transistor
print "  (PIN X=15 Y=1 R=U1.1 P=roundpad)"
print "  (PIN X=15 Y=2 R=U1.2 P=squarepad)"
print "  (PIN X=15 Y=3 R=U1.3 P=oblongpad)"
print "}"

print "{NET=padtst"
print "  (PAD X=16 Y=1 L=component S=ROUND SX=0.5 SY=0.5)"
print "  (PAD X=16 Y=2 L=component S=RECT SX=0.5 SY=0.5)"
print "  (PAD X=16 Y=3 L=component S=OBLONG SX=0.75 SY=0.5)"
print "  (PAD X=17 Y=1 L=solder S=ROUND SX=0.5 SY=0.5)"
print "  (PAD X=17 Y=2 L=solder S=RECT SX=0.5 SY=0.5)"
print "  (PAD X=17 Y=3 L=solder S=OBLONG SX=0.75 SY=0.5)"
print "}"

print "{NET=usegtst"
print "  (USEG X1=18 Y1=1 L1=component X2=19 Y2=1 L2=component ZL=component ZW=0.1 ZLEN=2 )"
print "  (USEG X1=18 Y1=2 L1=component X2=19 Y2=2 L2=component Z=50 D=1e-12 R=5 )"
print "  (USEG X1=18 Y1=3 L1=component X2=19 Y2=3 L2=solder ZL=component ZW=0.1 ZLEN=2 )"
print "  (USEG X1=18 Y1=4 L1=component X2=19 Y2=4 L2=solder Z=50 D=1e-12 R=5 )"
print "}"

x0 = 6.0
y0 = 5.0
l = 2.0
h = 0.5
r = h/2.0
w = h/4.0

print "{NET=polylinetst"
print "  {POLYGON L=component W=0.05 ID=1 X=%f Y=%f" % (x0-w, y0-w)
print "     (LINE X=%f Y=%f)" % (x0+w, y0-w)
print "     (LINE X=%f Y=%f)" % (x0+w, y0+w)
print "     (LINE X=%f Y=%f)" % (x0-w, y0+w)
print "     (LINE X=%f Y=%f)" % (x0-w, y0-w)
print "  }"
print "  {POLYLINE L=component W=0.1 ID=1 X=%f Y=%f" % (x0, y0)
for a in range (0, 8, 2):
  print "    (LINE X=%f Y=%f)" % ( x0+l, y0+a*h)
  print "    (CURVE X1=%f Y1=%f X2=%f Y2=%f XC=%f YC=%f R=%f)" % ( x0+l, y0+a*h, x0+l, y0+(a+1)*h, x0+l, y0+(a+0.5)*h, r)
  print "    (LINE X=%f Y=%f)" % ( x0, y0+(a+1)*h)
  print "    (CURVE X1=%f Y1=%f X2=%f Y2=%f XC=%f YC=%f R=%f)" % ( x0, y0+(a+2)*h, x0, y0+(a+1)*h, x0, y0+(a+1.5)*h, r)

print "  }"
print "}"

x0 = 9.5
y0 = 5.5
r = 0.5

print "{NET=curvetst"
print "  {POLYGON L=component W=0.05 ID=2 X=%f Y=%f" % (x0+r, y0)
print "     (CURVE X1=%f Y1=%f X2=%f Y2=%f XC=%f YC=%f R=%f )" % (x0+r, y0, x0+r, y0, x0, y0, r)
print "  }"
print "  {POLYVOID ID=2 X=%f Y=%f" % (x0-r/2, y0)
print "     (CURVE X1=%f Y1=%f X2=%f Y2=%f XC=%f YC=%f R=%f )" % (x0-r/2, y0, x0+r/2, y0, x0, y0, r/2)
print "     (LINE X=%f Y=%f)" % (x0-r/2, y0)
print "  }"
print "}"

x0 = 9.0
y0 = 6.5
w = 0.5

print "{NET=linetst"
print "  {POLYGON L=component W=0.05 ID=3 X=%f Y=%f" % (x0, y0)
print "     (LINE X=%f Y=%f)" % (x0+2*w, y0)
print "     (LINE X=%f Y=%f)" % (x0+2*w, y0+w)
print "     (LINE X=%f Y=%f)" % (x0+w, y0+w)
print "     (LINE X=%f Y=%f)" % (x0+w, y0+2*w)
print "     (LINE X=%f Y=%f)" % (x0, y0+2*w)
print "     (LINE X=%f Y=%f)" % (x0, y0)
print "  }"

x0 = 9.125
y0 = 6.625
w = 0.25

print "  {POLYVOID ID=3 X=%f Y=%f" % (x0, y0)
print "     (LINE X=%f Y=%f)" % (x0+2*w, y0)
print "     (LINE X=%f Y=%f)" % (x0+2*w, y0+w)
print "     (LINE X=%f Y=%f)" % (x0+w, y0+w)
print "     (LINE X=%f Y=%f)" % (x0+w, y0+2*w)
print "     (LINE X=%f Y=%f)" % (x0, y0+2*w)
print "     (LINE X=%f Y=%f)" % (x0, y0)
print "  }"

print "}"

x0 = 11.5
y0 = 5.0
w = 1.0
h = 0.5
s = 1.0

def mkpolygon (typ, linewdth, id, x0, y0, w, h):
  print "  {POLYGON L=component T=%s W=%f ID=%i X=%f Y=%f" % (typ, linewdth, id, x0, y0)
  print "     (LINE X=%f Y=%f)" % (x0+w, y0)
  print "     (LINE X=%f Y=%f)" % (x0+w, y0+h)
  print "     (LINE X=%f Y=%f)" % (x0, y0+h)
  print "     (LINE X=%f Y=%f)" % (x0, y0)
  print "  }"
  return

def mkpolyvoid (id, x0, y0, w, h):
  print "  {POLYVOID ID=%i X=%f Y=%f" % (id, x0, y0)
  print "     (LINE X=%f Y=%f)" % (x0+w, y0)
  print "     (LINE X=%f Y=%f)" % (x0+w, y0+h)
  print "     (LINE X=%f Y=%f)" % (x0, y0+h)
  print "     (LINE X=%f Y=%f)" % (x0, y0)
  print "  }"
  return

x1 = 11.0
x2 = x0 + w / 4.0
print "{NET=poly_clearance_tst PS=0.05"
print "  (VIA X=%f Y=%f P=0802pad)" % (x2, y0+h/2.0)
print "  (VIA X=%f Y=%f P=0802pad)" % (x2, y0+h/2.0+s)
print "  (VIA X=%f Y=%f P=0802pad)" % (x2, y0+h/2.0+2*s)
print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=0.04 L=component)" % (x1, y0+h/2.0, x2, y0+h/2.0)
print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=0.04 L=component)" % (x1, y0+h/2.0+s, x2, y0+h/2.0+s)
print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=0.04 L=component)" % (x1, y0+h/2.0+2*s, x2, y0+h/2.0+2*s)
print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=0.04 L=component)" % (x1, y0+h/2.0, x1, y0+h/2.0+s)
print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=0.04 L=component)" % (x1, y0+h/2.0+s, x1, y0+h/2.0+2*s)

mkpolygon("POUR", 0.1, 4, x0, y0, w, h)
mkpolygon("COPPER", 0.1, 5, x0, y0 + s, w, h)
mkpolygon("PLANE", 0.1, 6, x0, y0 + 2 * s, w, h)

print "}"

x1 = 13.0
x2 = x0 + w * 3.0 / 4.0
print "{NET=poly_clearance_tst2 PS=0.05"
print "  (VIA X=%f Y=%f P=0802pad)" % (x2, y0+h/2.0)
print "  (VIA X=%f Y=%f P=0802pad)" % (x2, y0+h/2.0+s)
print "  (VIA X=%f Y=%f P=0802pad)" % (x2, y0+h/2.0+2*s)
print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=0.04 L=component)" % (x1, y0+h/2.0, x2, y0+h/2.0)
print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=0.04 L=component)" % (x1, y0+h/2.0+s, x2, y0+h/2.0+s)
print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=0.04 L=component)" % (x1, y0+h/2.0+2*s, x2, y0+h/2.0+2*s)
print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=0.04 L=component)" % (x1, y0+h/2.0, x1, y0+h/2.0+s)
print "  (SEG X1=%f Y1=%f X2=%f Y2=%f W=0.04 L=component)" % (x1, y0+h/2.0+s, x1, y0+h/2.0+2*s)
print "}"

# test nesting of polygons

x0 = 14.0
y0 = 5.0
s = 0.25
print "{NET=nesting_poly_1 PS=0"
mkpolygon("POUR", 0, 7, x0, y0, 8*s, 8*s)
mkpolyvoid(7, x0+1*s, y0+1*s, 6*s, 6*s)
print "}"
print "{NET=nesting_poly_2 PS=0"
mkpolygon("POUR", 0, 8, x0+2*s, y0+2*s, 4*s, 4*s)
mkpolyvoid(8, x0+3*s, y0+3*s, 2*s, 2*s)
print "}"

print "{END}"

# not truncated
