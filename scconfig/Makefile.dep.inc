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
gsub /local/dep/CFLAGS {-isystem [.][.]} {-I ..}
gsub /local/dep/CFLAGS {-isystem [.]} {-I .}
sortuniq /local/dep/SRCS_SORTED /local/dep/SRCS

foreach /local/c in /local/dep/SRCS_SORTED
	put /local/o /local/c
	sub {/local/o} {.c$} {.o}
	switch /local/c
		case {/src_plugins/} end
		case {$(PLUGDIR)}
			print [@	gcc -MT @/local/o@ -MM @/local/c@ @/local/dep/CFLAGS@ @/local/pcb/DEPCFLAGS@ >> Makefile.dep
@]; end
		case {$(SRC_3RD_DIR)}
			print [@	gcc -MT @/local/o@ -MM @/local/c@ @/local/dep/CFLAGS@ >> Makefile.dep
@]; end
		case {../src_3rd/}
			print [@	gcc -MT @/local/o@ -MM @/local/c@ @/local/dep/CFLAGS@ >> Makefile.dep
@]; end

		default print [@	gcc -MM @/local/c@ @/local/dep/CFLAGS@ >> Makefile.dep
@]; end
	end
end
