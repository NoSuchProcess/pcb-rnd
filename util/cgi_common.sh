#!/bin/sh

# shell lib

url_decode()
{
awk '
	function cd(n)
	{
		chr=sprintf("%c", n);
		if (chr == "&")
			chr = "\\&"
		code="%" sprintf("%02x", n);
		CODE[tolower(code)] = chr;
		CODE[toupper(code)] = chr;
	}

	BEGIN {
		for(n = 1; n < 256; n++)
			cd(n);
	}

	{
		tmp = $0;
		gsub("[+]", " ", tmp);
		for(c in CODE) {
			gsub(c, CODE[c], tmp)
		}
		print tmp
	}
'
}

error()
{
	echo "Content-type: text/plain"
	echo ""
	echo "Error: $*"
	exit 0
}

radio()
{
	local chk
	if test "$3" = "$2"
	then
		chk=" checked=\"true\""
	fi
	echo "<input type=\"radio\" name=\"$1\" value=\"$2\"$chk>"
}

checked()
{
	if test ! -z "$1"
	then
		echo " checked=\"true\""
	fi
}

fix_ltgt()
{
	sed "s/</\&lt;/g;s/>/\&gt;/g"
}

cgi_png()
{
	echo "Content-type: image/png"
	echo ""
	cparm=""
	if test ! -z "$QS_mm"
	then
		cparm="$cparm --mm"
	fi
	if test ! -z "$QS_grid"
	then
		cparm="$cparm --grid-unit $QS_grid"
	fi
	if test ! -z "$QS_annotation"
	then
		annot=$QS_annotation
	fi
	if test ! -z "$QS_diamond"
	then
		cparm="$cparm --diamond"
	fi
	if test ! -z "$QS_photo"
	then
		cparm="$cparm --photo"
	fi
	if test ! -z "$QS_dimvalue"
	then
		annot="$annot:dimvalue"
	fi
	if test ! -z "$QS_dimname"
	then
		annot="$annot:dimname"
	fi
	if test ! -z "$QS_pins"
	then
		annot="$annot:pins"
	fi
	if test ! -z "$QS_background"
	then
		annot="$annot:background"
	fi
	case "$QS_thumb"
	in
		1) animarg="-x 64 -y 48" ;;
		2) animarg="-x 128 -y 96" ;;
		3) animarg="-x 192 -y 144" ;;
		*) animarg="" ;;
	esac
	if test ! -z  "$annot"
	then
			cparm="$cparm --annotation $annot"
	fi
	(echo "$fptext" | $fp2anim $cparm; echo 'screenshot "/dev/stdout"') | $animator -H $animarg
}
