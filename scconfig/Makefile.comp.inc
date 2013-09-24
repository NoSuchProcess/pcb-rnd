# Generate Makefile code to compile .c -> .o
# Arguments:
#  /local/comp/OBJS    list of .o files (assuming each have at least a .c)

print {
### explicit rules for .c -> .o ###
}

foreach /local/o in /local/comp/OBJS
put /local/c /local/o
sub /local/c {.o$} {.c}
print [@
@/local/o@: @/local/c@
	$(CC) -c $(CFLAGS) -o @/local/o@ $<
@]
end
