BEGIN {
	init_bar_db()

	help_auto()

	set_arg(P, "?dit_width", "0.5mm")
        set_arg(P, "?page", "B")

	proc_args(P, "text,dit_width,page", "text")

	W = parse_dim(P["dit_width"])
	XS=W
	Y1=0

	text_x = XS*11
	text_y = W
        element_begin("", P["text"], "morse(" P["text"] "," P["dit_width"] "," P["page"] ")"  , 0, 0, text_x, text_y, 0, W/5)
	morse(P["page"], P["text"])
	element_end()
}


# main entry point for the actual code generation
function morse(page, text    ,sum,v,sm,n,pos)
{
	x = 0
	v = length(text)
	pos = 1
	for(n = 1; n <= v; n++) {
 		sm = gen_chr(page, substr(text, n, 1))
		if (sm == "")
			continue
		sum += sm * pos
		pos++
	}
}

function dit(x)
{
	element_line(x*XS, Y1, x*XS, Y1, W)
}

function dahline(x)
{
	element_line(x*XS, Y1, (x+2)*XS, Y1, W)
}

function gen_val(tbl, val,        bits) {
	bits=ENC[val]
	print "#", val, bits
	for(i = 1; i <= length(bits); i++) {
		if (substr(bits, i, 1) == "1") {
			dit(x);
			x++;
		} else if (substr(bits, i, 1) == "3") {
			dahline(x);
			x+=3;
		} else {
			x++; # space
		}
	}
}

function gen_chr(tbl, chr    ,val)
{
	val=CHR[tbl, chr]
	if (val == "")
		return ""
	gen_val(tbl, val)
	return val
}


function init_bar_db() {
# code 128 bar bits
# CHR[codepage, character]=code
# ENC[code] = bar_bits
	CHR["A", " "]=0
	CHR["B", " "]=0
	ENC[0]="000000"
	CHR["A", "U"]=53
	CHR["B", "U"]=53
	ENC[53]="10103000"
	CHR["A", "!"]=1
	CHR["B", "!"]=1
	ENC[1]="30103010303000"
	CHR["A", "V"]=54
	CHR["B", "V"]=54
	ENC[54]="1010103000"
	CHR["A", "\""]=2
	CHR["B", "\""]=2
	ENC[2]="10301010301000"
	CHR["A", "W"]=55
	CHR["B", "W"]=55
	ENC[55]="10303000"
	CHR["A", "#"]=3
	CHR["B", "#"]=3
	ENC[3]=""
	CHR["A", "X"]=56
	CHR["B", "X"]=56
	ENC[56]="3010103000"
	CHR["A", "$"]=4
	CHR["B", "$"]=4
	ENC[4]="1010103010103000"
	CHR["A", "Y"]=57
	CHR["B", "Y"]=57
	ENC[57]="3010303000"
	CHR["A", "%"]=5
	CHR["B", "%"]=5
	ENC[5]=""
	CHR["A", "Z"]=58
	CHR["B", "Z"]=58
	ENC[58]="3030101000"
	CHR["A", "&"]=6
	CHR["B", "&"]=6
	ENC[6]="103010101000"
	CHR["A", "["]=59
	CHR["B", "["]=59
	ENC[59]=""
	CHR["A", "'"]=7
	CHR["B", "'"]=7
	ENC[7]="10303030301000"
	CHR["A", "\\"]=60
	CHR["B", "\\"]=60
	ENC[60]="11101111010"
	CHR["A", "("]=8
	CHR["B", "("]=8
	ENC[8]="301030301000"
	CHR["A", "]"]=61
	CHR["B", "]"]=61
	ENC[61]=""
	CHR["A", ")"]=9
	CHR["B", ")"]=9
	ENC[9]="30103030103000"
	CHR["A", "^"]=62
	CHR["B", "^"]=62
	ENC[62]=""
	CHR["A", "*"]=10
	CHR["B", "*"]=10
	ENC[10]=""
	CHR["A", "_"]=63
	CHR["B", "_"]=63
	ENC[63]="10103030103000"
	CHR["A", "+"]=11
	CHR["B", "+"]=11
	ENC[11]="103010301000"
	CHR["A", "NUL"]=64
	CHR["B", "`"]=64
	ENC[64]=""
	CHR["A", ","]=12
	CHR["B", ","]=12
	ENC[12]="30301010303000"
	CHR["A", "SOH"]=65
	CHR["B", "a"]=65
	ENC[65]="103000"
	CHR["A", "-"]=13
	CHR["B", "-"]=13
	ENC[13]="30101010103000"
	CHR["A", "STX"]=66
	CHR["B", "b"]=66
	ENC[66]="3010101000"
	CHR["A", "."]=14
	CHR["B", "."]=14
	ENC[14]="30103010301000"
	CHR["A", "ETX"]=67
	CHR["B", "c"]=67
	ENC[67]="3010301000"
	CHR["A", "/"]=15
	CHR["B", "/"]=15
	ENC[15]="301010301000"
	CHR["A", "EOT"]=68
	CHR["B", "d"]=68
	ENC[68]="30101000"
	CHR["A", "0"]=16
	CHR["B", "0"]=16
	ENC[16]="303030303000"
	CHR["A", "ENQ"]=69
	CHR["B", "e"]=69
	ENC[69]="1000"
	CHR["A", "1"]=17
	CHR["B", "1"]=17
	ENC[17]="103030303000"
	CHR["A", "ACK"]=70
	CHR["B", "f"]=70
	ENC[70]="1010301000"
	CHR["A", "2"]=18
	CHR["B", "2"]=18
	ENC[18]="101030303000"
	CHR["A", "BEL"]=71
	CHR["B", "g"]=71
	ENC[71]="30301000"
	CHR["A", "3"]=19
	CHR["B", "3"]=19
	ENC[19]="101010303000"
	CHR["A", "BS"]=72
	CHR["B", "h"]=72
	ENC[72]="1010101000"
	CHR["A", "4"]=20
	CHR["B", "4"]=20
	ENC[20]="101010103000"
	CHR["A", "HT"]=73
	CHR["B", "i"]=73
	ENC[73]="101000"
	CHR["A", "5"]=21
	CHR["B", "5"]=21
	ENC[21]="101010101000"
	CHR["A", "LF"]=74
	CHR["B", "j"]=74
	ENC[74]="1030303000"
	CHR["A", "6"]=22
	CHR["B", "6"]=22
	ENC[22]="301010101000"
	CHR["A", "VT"]=75
	CHR["B", "k"]=75
	ENC[75]="30103000"
	CHR["A", "7"]=23
	CHR["B", "7"]=23
	ENC[23]="303010101000"
	CHR["A", "FF"]=76
	CHR["B", "l"]=76
	ENC[76]="1030101000"
	CHR["A", "8"]=24
	CHR["B", "8"]=24
	ENC[24]="303030101000"
	CHR["A", "CR"]=77
	CHR["B", "m"]=77
	ENC[77]="303000"
	CHR["A", "9"]=25
	CHR["B", "9"]=25
	ENC[25]="303030301000"
	CHR["A", "SO"]=78
	CHR["B", "n"]=78
	ENC[78]="301000"
	CHR["A", ":"]=26
	CHR["B", ":"]=26
	ENC[26]="30303010101000"
	CHR["A", "SI"]=79
	CHR["B", "o"]=79
	ENC[79]="30303000"
	CHR["A", ";"]=27
	CHR["B", ";"]=27
	ENC[27]="30103010301000"
	CHR["A", "DLE"]=80
	CHR["B", "p"]=80
	ENC[80]="1030301000"
	CHR["A", ""]=28
	CHR["B", ""]=28
	ENC[28]="000"
	CHR["A", "DC1"]=81
	CHR["B", "q"]=81
	ENC[81]="3030103000"
	CHR["A", "="]=29
	CHR["B", "="]=29
	ENC[29]="301010103000"
	CHR["A", "DC2"]=82
	CHR["B", "r"]=82
	ENC[82]="10301000"
	CHR["A", ">"]=30
	CHR["B", ">"]=30
	ENC[30]=""
	CHR["A", "DC3"]=83
	CHR["B", "s"]=83
	ENC[83]="10101000"
	CHR["A", "?"]=31
	CHR["B", "?"]=31
	ENC[31]="10103030101000"
	CHR["A", "DC4"]=84
	CHR["B", "t"]=84
	ENC[84]="3000"
	CHR["A", "@"]=32
	CHR["B", "@"]=32
	ENC[32]="10303010301000"
	CHR["A", "NAK"]=85
	CHR["B", "u"]=85
	ENC[85]="10103000"
	CHR["A", "A"]=33
	CHR["B", "A"]=33
	ENC[33]="103000"
	CHR["A", "SYN"]=86
	CHR["B", "v"]=86
	ENC[86]="1010103000"
	CHR["A", "B"]=34
	CHR["B", "B"]=34
	ENC[34]="3010101000"
	CHR["A", "ETB"]=87
	CHR["B", "w"]=87
	ENC[87]="30301000"
	CHR["A", "C"]=35
	CHR["B", "C"]=35
	ENC[35]="3010301000"
	CHR["A", "CAN"]=88
	CHR["B", "x"]=88
	ENC[88]="3010103000"
	CHR["A", "D"]=36
	CHR["B", "D"]=36
	ENC[36]="30101000"
	CHR["A", "EM"]=89
	CHR["B", "y"]=89
	ENC[89]="3010303000"
	CHR["A", "E"]=37
	CHR["B", "E"]=37
	ENC[37]="1000"
	CHR["A", "SUB"]=90
	CHR["B", "z"]=90
	ENC[90]="3030101000"
	CHR["A", "F"]=38
	CHR["B", "F"]=38
	ENC[38]="1010301000"
	CHR["A", "ESC"]=91
	CHR["B", "{"]=91
	ENC[91]=""
	CHR["A", "G"]=39
	CHR["B", "G"]=39
	ENC[39]="10303000"
	CHR["A", "FS"]=92
	CHR["B", "|"]=92
	ENC[92]=""
	CHR["A", "H"]=40
	CHR["B", "H"]=40
	ENC[40]="1010101000"
	CHR["A", "GS"]=93
	CHR["B", "}"]=93
	ENC[93]=""
	CHR["A", "I"]=41
	CHR["B", "I"]=41
	ENC[41]="1000"
	CHR["A", "RS"]=94
	CHR["B", "~"]=94
	ENC[94]=""
	CHR["A", "J"]=42
	CHR["B", "J"]=42
	ENC[42]="1030303000"
	CHR["A", "US"]=95
	CHR["B", "DEL"]=95
	ENC[95]=""
	CHR["A", "K"]=43
	CHR["B", "K"]=43
	ENC[43]="30103000"
	CHR["A", "FNC3"]=96
	CHR["B", "FNC3"]=96
	ENC[96]=""
	CHR["A", "L"]=44
	CHR["B", "L"]=44
	ENC[44]="1030101000"
	CHR["A", "FNC2"]=97
	CHR["B", "FNC2"]=97
	ENC[97]=""
	CHR["A", "M"]=45
	CHR["B", "M"]=45
	ENC[45]="10111011000"
	CHR["A", "SHIFT"]=98
	CHR["B", "SHIFT"]=98
	ENC[98]=""
	CHR["A", "N"]=46
	CHR["B", "N"]=46
	ENC[46]="10111000110"
	CHR["A", "CodeC"]=99
	CHR["B", "CodeC"]=99
	ENC[99]=""
	CHR["A", "O"]=47
	CHR["B", "O"]=47
	ENC[47]="30303000"
	CHR["A", "CodeB"]=100
	CHR["B", "FNC4"]=100
	ENC[100]=""
	CHR["A", "P"]=48
	CHR["B", "P"]=48
	ENC[48]="1030301000"
	CHR["A", "FNC4"]=101
	CHR["B", "CodeA"]=101
	ENC[101]=""
	CHR["A", "Q"]=49
	CHR["B", "Q"]=49
	ENC[49]="3030103000"
	CHR["A", "FNC1"]=102
	CHR["B", "FNC1"]=102
	ENC[102]=""
	CHR["A", "R"]=50
	CHR["B", "R"]=50
	ENC[50]="10301000"
	CHR["A", "STARTA"]=103
	CHR["B", "STARTA"]=103
	ENC[103]=""
	CHR["A", "S"]=51
	CHR["B", "S"]=51
	ENC[51]="10101000"
	CHR["A", "STARTB"]=104
	CHR["B", "STARTB"]=104
	ENC[104]=""
	CHR["A", "T"]=52
	CHR["B", "T"]=52
	ENC[52]="3000"
	CHR["A", "STARTC"]=105
	CHR["B", "STARTC"]=105
	ENC[105]=""
	CHR["A", " "]=0
	CHR["B", "STOP"]=0
	ENC[0]="103010101000"
	CHR["A", "END"]=500
	CHR["B", "END"]=500
	ENC[500]="10101030103000"
}

