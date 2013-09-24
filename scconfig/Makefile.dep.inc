# Generate Makefile code that can generate Makefile.dep
# Arguments:
#  /local/dep/CFLAGS   CFLAGS used for compiling
#  /local/dep/SRCS     list of c soures

print [@

### generate dependencies (requires gcc) ###
FORCE:

include Makefile.dep

dep: FORCE
	echo "### Generated file, do not edit, run make dep ###" > Makefile.dep
	echo "" >> Makefile.dep
@]

gsub /local/dep/CFLAGS {-I} {-isystem }
foreach /local/c in /local/dep/SRCS
	print [@	gcc -MM @/local/c@ @/local/dep/CFLAGS@ >> Makefile.dep
@]
end
