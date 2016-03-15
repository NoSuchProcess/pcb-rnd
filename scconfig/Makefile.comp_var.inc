# Generate Makefile code to compile .c -> .o
# Arguments:
#  /local/comp/OBJS    list of .o files (assuming each have at least a .c)

put /local/comp/output {}
append /local/comp/CFLAGS {}

foreach /local/o in /local/comp/OBJS
put /local/c /local/o
sub /local/c {.o$} {.c}
append /local/comp/output [@
@/local/o@: @/local/c@
	$(CC) -c $(CFLAGS) @/local/comp/CFLAGS@ -o @/local/o@ $<
@]
end

put /local/comp/OBJS {}
put /local/comp/CFLAGS {}

