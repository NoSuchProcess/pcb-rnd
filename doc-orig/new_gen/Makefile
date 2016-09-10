all: fractional_size.html letter_size.html metric_size.html wire_size.html

fractional_size.html: fractional_size.tab tab2html.sh
	./tab2html.sh 4 < fractional_size.tab >$@

letter_size.html: letter_size.tab tab2html.sh
	./tab2html.sh 4 < letter_size.tab >$@

metric_size.html: metric_size.tab tab2html.sh
	./tab2html.sh 4 < metric_size.tab >$@

wire_size.html: wire_size.tab tab2html.sh
	./tab2html.sh 4 < wire_size.tab >$@

