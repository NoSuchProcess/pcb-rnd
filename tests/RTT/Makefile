E = ./Export.sh -t -a

all:

test:
	@make test_export && echo "*** export: QC PASS ***"

test_export:
	@$E -f bboard ; \
	$E -f nelma ; \
	$E -f bom ; \
	$E -f dsn 2>/dev/null; \
	$E -f IPC-D-356 ; \
	$E -f ps ; \
	$E -f XY ; \
	$E -f openscad ; \
	$E -f png ; \
	$E -f gerber ; \
	$E -f svg
