Element(0x00 "Molex .025 pitch 20 pin plug" "" "MOLEX_025 20" 0 0 3 100 0x00)
(
		Pad(54 0  111 0  14 "1" 0x00)
		Pad(-111 0  -54 0  14 "2" 0x100)
		Pad(54 25  111 25  14 "3" 0x100)
		Pad(-111 25  -54 25  14 "4" 0x100)
		Pad(54 50  111 50  14 "5" 0x100)
		Pad(-111 50  -54 50  14 "6" 0x100)
		Pad(54 75  111 75  14 "7" 0x100)
		Pad(-111 75  -54 75  14 "8" 0x100)
		Pad(54 100  111 100  14 "9" 0x100)
		Pad(-111 100  -54 100  14 "10" 0x100)
		Pad(54 125  111 125  14 "11" 0x100)
		Pad(-111 125  -54 125  14 "12" 0x100)
		Pad(54 150  111 150  14 "13" 0x100)
		Pad(-111 150  -54 150  14 "14" 0x100)
		Pad(54 175  111 175  14 "15" 0x100)
		Pad(-111 175  -54 175  14 "16" 0x100)
		Pad(54 200  111 200  14 "17" 0x100)
		Pad(-111 200  -54 200  14 "18" 0x100)
		Pad(54 225  111 225  14 "19" 0x100)
		Pad(-111 225  -54 225  14 "20" 0x100)
	# Keying is done with two sizes of alignment pins: 35 and 28 mils
	Pin(0 -50 50 35 "M1" 0x01)
	Pin(0 275 43 28 "M2" 0x01)
	# ends of mounting pads are 71 and 169 mils from end pad centers
	Pad(0 -110 0 -130 79 "M3" 0x100)
	Pad(0 335 0 355 79 "M4" 0x100)
	ElementLine(-100 -150   50 -150 10)
	ElementLine(  50 -150  100 -100 10)
	ElementLine( 100 -100  100 375 10)
	ElementLine( 100 375 -100 375 10)
	ElementLine(-100 375 -100 -150 10)
	# Support for aggregate parts built from this base, like
	# the nanoEngine below.
)
