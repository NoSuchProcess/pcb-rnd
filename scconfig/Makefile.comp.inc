# Generate Makefile code to compile .c -> .o
# Arguments:
#  /local/comp/OBJS     list of .o files (assuming each have at least a .c)
#  /local/comp/OBJS_C99 list of .o files (assuming each have at least a .c), for non-c89 compilation

print {
### explicit rules for .c -> .o ###
}

append /local/comp/CFLAGS {}

foreach /local/o in /local/comp/OBJS
put /local/c /local/o
sub /local/c {.o$} {.c}
print [@
@/local/o@: @/local/c@
	$(CC) -c $(C89FLAGS) @/local/comp/CFLAGS@ -o @/local/o@ $<
@]
end

append /local/comp/OBJS_C99 {}
foreach /local/o in /local/comp/OBJS_C99
put /local/c /local/o
sub /local/c {.o$} {.c}
print [@
@/local/o@: @/local/c@
	$(CC) -c $(CFLAGS) @/local/comp/CFLAGS@ -o @/local/o@ $<
@]
end

put /local/comp/OBJS {}
put /local/comp/OBJS_C99 {}
put /local/comp/CFLAGS {}

