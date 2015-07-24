##from:pcb
##geo:90
##geo:male

Element(0x00 "DSUB connector, female/male" "" "DB9M" 1000 1702 1 150 0x00)
(
	# Gehaeuse (schmaler Kasten incl. Bohrungen)
	ElementLine(635 880 665 880 10)
	ElementLine(665 880 665 2092 10)
	ElementLine(665 2092 635 2092 10)
	ElementLine(635 2092 635 880 10)
	ElementLine(635 940 665 940 10)
	ElementLine(635 1060 665 1060 10)
	ElementLine(635 2032 665 2032 10)
	ElementLine(635 1912 665 1912 10)
	# Gehaeuse (aeusserer Kasten)
	# This part of the connector normally hangs off the circuit board,
	# so it is confusing to actually mark it on the silkscreen
	# define(`X1', `eval(BASEX-PANEL_DISTANCE-260)')
	# define(`Y1', `eval(PY1-100)')
	# define(`X2', `eval(BASEX-PANEL_DISTANCE)')
	# define(`Y2', `eval(PY2+100)')
	# ElementLine(X1 Y1 X2 Y1 20)
	# ElementLine(X2 Y1 X2 Y2 10)
	# ElementLine(X2 Y2 X1 Y2 20)
	# ElementLine(X1 Y2 X1 Y1 20)
	# Gehaeuse (innerer Kasten)
	ElementLine(665 1110 770 1110 20)
	ElementLine(770 1110 770 1862 20)
	ElementLine(770 1862 665 1862 20)
	ElementLine(665 1862 665 1110 10)
	# Pins
		# First row
			Pin(1056 1270 60 35 "1" 0x101)
			ElementLine(1016 1270 770 1270 20)
			Pin(1056 1378 60 35 "2" 0x01)
			ElementLine(1016 1378 770 1378 20)
			Pin(1056 1486 60 35 "3" 0x01)
			ElementLine(1016 1486 770 1486 20)
			Pin(1056 1594 60 35 "4" 0x01)
			ElementLine(1016 1594 770 1594 20)
		# Last pin in first row
		Pin(1056 1702 60 35 "5" 0x01)
		ElementLine(1016 1702 770 1702 20)
		# Second row
			Pin(944 1324 60 35 "6" 0x01)
			ElementLine(904 1324 770 1324 20)
			Pin(944 1432 60 35 "7" 0x01)
			ElementLine(904 1432 770 1432 20)
			Pin(944 1540 60 35 "8" 0x01)
			ElementLine(904 1540 770 1540 20)
			Pin(944 1648 60 35 "9" 0x01)
			ElementLine(904 1648 770 1648 20)
		# Plazierungsmarkierung == PIN 1
		Mark(1050 1270)
	# Befestigungsbohrung
	Pin(1000  1000 250 125 "C1" 0x01)
	Pin(1000 1972 250 125 "C2" 0x01)
)
