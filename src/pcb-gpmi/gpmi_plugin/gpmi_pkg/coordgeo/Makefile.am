# This file is package-specific. Do include it in the distribution.
#
# GPMI fingerprint: Makefile.am.ext-pkg

# End INST_PATH with /
INST_PATH = pcb-gpmi/
FILE_NAME = coordgeo

SOURCE  = coordgeo.c
HEADER  = coordgeo.h

PKG_LDFLAGS = -lm

include ../../../Makefile.config
INCLUDES = -I $(PCB_SRC)

test: test.o coordgeo.o
