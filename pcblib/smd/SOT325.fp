##from:pcb
##for:transistor
##for:stabilizer
##for:linear


	                             # 78 for SOT23
	                             # 82 for SOT23
	      # 41 for SOT23
	               # 34 for SOT23, 24 for SOT25
# alias sc70-5
Element(0x00 "SMT transistor, 5 pins" "" "SOT325" 100 0 3 100 0x00)
(
	ElementLine(0 0 0 119 10)
	ElementLine(0 119 80 119 10)
	ElementLine(80 119 80 0 10)
	ElementLine(80 0 0 0 10)
	# 1st side, 1st pin
	Pad(14 84
	       14 104
			   15 "1" "1" 0x100)
	# 1st side, 2nd pin
	# 1st side, 3rd pin
	Pad(65 84
	    65 104
			   15 "2" "2" 0x100)
	# 2nd side, 3rd pin
	Pad(65 14
	       65 34
			   15 "3" "3" 0x100)
	# 2nd side, 2nd pin
	Pad(40 14
	       40 34
			   15 "4" "4" 0x100)
	# 2nd side, 1st pin
	Pad(14 14
	       14 34
			   15 "5" "5" 0x100)
	Mark(14 94)
)
