Element(0x00 "DIN41.612 row a+c female" "" "DIN41_612FEMALE ac" 50 100 3 200 0x00)
(
	# Reihe a
	Pin(200 400 60 30 "a1" 0x101)
	Pin(200 500 60 30 "a2" 0x01)
	Pin(200 600 60 30 "a3" 0x01)
	Pin(200 700 60 30 "a4" 0x01)
	Pin(200 800 60 30 "a5" 0x01)
	Pin(200 900 60 30 "a6" 0x01)
	Pin(200 1000 60 30 "a7" 0x01)
	Pin(200 1100 60 30 "a8" 0x01)
	Pin(200 1200 60 30 "a9" 0x01)
	Pin(200 1300 60 30 "a10" 0x01)
	Pin(200 1400 60 30 "a11" 0x01)
	Pin(200 1500 60 30 "a12" 0x01)
	Pin(200 1600 60 30 "a13" 0x01)
	Pin(200 1700 60 30 "a14" 0x01)
	Pin(200 1800 60 30 "a15" 0x01)
	Pin(200 1900 60 30 "a16" 0x01)
	Pin(200 2000 60 30 "a17" 0x01)
	Pin(200 2100 60 30 "a18" 0x01)
	Pin(200 2200 60 30 "a19" 0x01)
	Pin(200 2300 60 30 "a20" 0x01)
	Pin(200 2400 60 30 "a21" 0x01)
	Pin(200 2500 60 30 "a22" 0x01)
	Pin(200 2600 60 30 "a23" 0x01)
	Pin(200 2700 60 30 "a24" 0x01)
	Pin(200 2800 60 30 "a25" 0x01)
	Pin(200 2900 60 30 "a26" 0x01)
	Pin(200 3000 60 30 "a27" 0x01)
	Pin(200 3100 60 30 "a28" 0x01)
	Pin(200 3200 60 30 "a29" 0x01)
	Pin(200 3300 60 30 "a30" 0x01)
	Pin(200 3400 60 30 "a31" 0x01)
	Pin(200 3500 60 30 "a32" 0x01)
	# Reihe b
	# Reihe c
		Pin(400 400 60 30 "c1" 0x01)
	Pin(400 500 60 30 "c2" 0x01)
	Pin(400 600 60 30 "c3" 0x01)
	Pin(400 700 60 30 "c4" 0x01)
	Pin(400 800 60 30 "c5" 0x01)
	Pin(400 900 60 30 "c6" 0x01)
	Pin(400 1000 60 30 "c7" 0x01)
	Pin(400 1100 60 30 "c8" 0x01)
	Pin(400 1200 60 30 "c9" 0x01)
	Pin(400 1300 60 30 "c10" 0x01)
	Pin(400 1400 60 30 "c11" 0x01)
	Pin(400 1500 60 30 "c12" 0x01)
	Pin(400 1600 60 30 "c13" 0x01)
	Pin(400 1700 60 30 "c14" 0x01)
	Pin(400 1800 60 30 "c15" 0x01)
	Pin(400 1900 60 30 "c16" 0x01)
	Pin(400 2000 60 30 "c17" 0x01)
	Pin(400 2100 60 30 "c18" 0x01)
	Pin(400 2200 60 30 "c19" 0x01)
	Pin(400 2300 60 30 "c20" 0x01)
	Pin(400 2400 60 30 "c21" 0x01)
	Pin(400 2500 60 30 "c22" 0x01)
	Pin(400 2600 60 30 "c23" 0x01)
	Pin(400 2700 60 30 "c24" 0x01)
	Pin(400 2800 60 30 "c25" 0x01)
	Pin(400 2900 60 30 "c26" 0x01)
	Pin(400 3000 60 30 "c27" 0x01)
	Pin(400 3100 60 30 "c28" 0x01)
	Pin(400 3200 60 30 "c29" 0x01)
	Pin(400 3300 60 30 "c30" 0x01)
	Pin(400 3400 60 30 "c31" 0x01)
	Pin(400 3500 60 30 "c32" 0x01)
	# Befestigungsbohrung
	Pin(290  180 120 80 "M1" 0x01)
	Pin(290 3720 120 80 "M2" 0x01)
	# Aeussere Begrenzung
	ElementLine( 80  80 520   80 20)
	ElementLine(520  80 520 3820 20)
	ElementLine(520 3820 80 3820 20)
	ElementLine( 80 3820 80   80 20)
	# Innere Begrenzung
	ElementLine(120  320 350  320 10)
	ElementLine(350  320 350  360 10)
	ElementLine(350  360 480  360 10)
	ElementLine(480  360 480 3540 10)
	ElementLine(480 3540 350 3540 10)
	ElementLine(350 3540 350 3580 10)
	ElementLine(350 3580 120 3580 10)
	ElementLine(120 3580 120  320 10)
	# Markierung: Pin 1a
	Mark(200 400)
)
