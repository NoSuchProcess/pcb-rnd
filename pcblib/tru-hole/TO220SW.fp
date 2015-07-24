##from:pcb
##for:transistor
##for:linear
##for:stabilizer
##geo:standing
	Element(0x00 "Transistor" "" "TO220SW" 0 10 0 100 0x00)
(
	Pin(100 200 90 60 "1" 0x101)
	Pin(200 300 90 60 "2" 0x01)
	Pin(300 200 90 60 "3" 0x01)
	# Gehaeuse
	ElementLine(  0  80 400  80 20)
	ElementLine(400  80 400 260 20)
	ElementLine(400 260   0 260 20) 
	ElementLine(  0 260   0  80 20) 
	# Kuehlfahne icl. Bohrung
	ElementLine(  0  80 400  80 20)
	ElementLine(400  80 400 140 20)
	ElementLine(400 140   0 140 20)
	ElementLine(  0 140   0  80 20)
	ElementLine(130 80 130 140 10)
	ElementLine(270 80 270 140 10)
	# Anschlussdraht
	ElementLine(200 300 200 260 30)
	Mark(100 200)
)
