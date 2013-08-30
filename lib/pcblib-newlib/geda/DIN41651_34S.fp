Element(0x00 "Header connector with latches" "" "DIN41651_34S" 50 100 3 200 0x00)
(
	Pin(200 500 60 40 "1" 0x101)
		Pin(300 500 60 40 "2" 0x01)
	Pin(200 600 60 40 "3" 0x01)
		Pin(300 600 60 40 "4" 0x01)
	Pin(200 700 60 40 "5" 0x01)
		Pin(300 700 60 40 "6" 0x01)
	Pin(200 800 60 40 "7" 0x01)
		Pin(300 800 60 40 "8" 0x01)
	Pin(200 900 60 40 "9" 0x01)
		Pin(300 900 60 40 "10" 0x01)
	Pin(200 1000 60 40 "11" 0x01)
		Pin(300 1000 60 40 "12" 0x01)
	Pin(200 1100 60 40 "13" 0x01)
		Pin(300 1100 60 40 "14" 0x01)
	Pin(200 1200 60 40 "15" 0x01)
		Pin(300 1200 60 40 "16" 0x01)
	Pin(200 1300 60 40 "17" 0x01)
		Pin(300 1300 60 40 "18" 0x01)
	Pin(200 1400 60 40 "19" 0x01)
		Pin(300 1400 60 40 "20" 0x01)
	Pin(200 1500 60 40 "21" 0x01)
		Pin(300 1500 60 40 "22" 0x01)
	Pin(200 1600 60 40 "23" 0x01)
		Pin(300 1600 60 40 "24" 0x01)
	Pin(200 1700 60 40 "25" 0x01)
		Pin(300 1700 60 40 "26" 0x01)
	Pin(200 1800 60 40 "27" 0x01)
		Pin(300 1800 60 40 "28" 0x01)
	Pin(200 1900 60 40 "29" 0x01)
		Pin(300 1900 60 40 "30" 0x01)
	Pin(200 2000 60 40 "31" 0x01)
		Pin(300 2000 60 40 "32" 0x01)
	Pin(200 2100 60 40 "33" 0x01)
		Pin(300 2100 60 40 "34" 0x01)
	# aeusserer Rahmen
	ElementLine(90 70 410 70 20)
	ElementLine(410 70 410 2530 20)
	ElementLine(410 2530 90 2530 20)
	ElementLine(90 2530 90 70 20)
	# innerer Rahmen mit Codieraussparung
	ElementLine(110  350 390  350 5)
	ElementLine(390  350 390 2250 5)
	ElementLine(390 2250 110 2250 5)
	ElementLine(110 2250 110 1375 5)
	ElementLine(110 1375  90 1375 5)
	ElementLine(90  1225 110 1225 5)
	ElementLine(110 1225 110  350 5)
	# Markierung Pin 1
	ElementLine(110 390 150 350 5)
	# Auswurfhebel oben
	ElementLine(200 70 200 350 5)
	ElementLine(300 70 300 350 5)
	# Auswurfhebel unten
	ElementLine(200 2250 200 2530 5)
	ElementLine(300 2250 300 2530 5)
	# Plazierungsmarkierung == Pin 1
	Mark(200 500)
)
