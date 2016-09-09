## -*- makefile -*-
##
##                            COPYRIGHT
##
##  PCB, interactive printed circuit board design
##  Copyright (C) 1994,1995,1996 Thomas Nau
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 2 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##
##  Contact addresses for paper mail and Email:
##  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
##  Thomas.Nau@rz.uni-ulm.de
##

## not until it is fully fixed up in terms of building
##SUBDIRS=	gs

info_TEXINFOS=	pcb.texi
pcb_TEXINFOS=	${inline_texi} ${pcb_output} ${tab_texi} ${images_output}

dist_html_DATA=	${html_docs} ${pcb_output_inst} ${images_output_inst}

dist_man_MANS=	pcb.1

html_docs=	${info_TEXINFOS:.texi=.html}

DVIS=

# put the html manual into 1 file instead of multiple files?
AM_MAKEINFOHTMLFLAGS=	--css-include=pcb.css --no-split

EXTRA_DIST= \
	ascii2texi.awk \
	eps2png \
	extract-docs \
	refcard.tex \
	pcb.css \
	${inline_texi} \
	${pcb_files} \
	${pcb_output_noinst} \
	${tab_files} \
	${tab_texi} \
	${images}

all:
	@echo "***"
	@echo "*** Compiling the old docs is disabled. We keep last compilation as static doc to edit until it is replaced with the new docs. ***"
	@echo "***"
	@false

old_all: $(html_docs)

# use this to avoid having the clean target delete pcb.{dvi,html,pdf,ps}
# which unfortunately deletes files which are part of the distfile
clean:
	-rm -rf pcb.aux pcb.cp pcb.cps pcb.fn pcb.fns pcb.ky pcb.kys pcb.log pcb.pg \
	pcb.pgs pcb.tmp pcb.toc pcb.tp pcb.tps pcb.vr pcb.vrs 


BUILT_SOURCES=	${tab_texi} ${pcb_output} ${inline_texi}
CLEANFILES= \
	refcard.aux refcard.log refcard.pdf \
	${pcb_output} ${tab_texi} ${inline_texi} pcb.html \
	${images_output}

inline_texi=	\
	options.texi \
	actions.texi \
	pcbfile.texi

ASCII2TEXI=	${AWK} -f $(srcdir)/ascii2texi.awk ncol=3

# Tables
tab_files=	\
	fractional_size.tab \
	letter_size.tab \
	metric_size.tab \
	wire_size.tab
tab_texi=	${tab_files:.tab=.texi}

# PCB Drawings
pcb_files=	\
	pad.pcb \
	puller.pcb \
	thermal.pcb \
	gcode.pcb

pcb_output_noinst=	\
	${pcb_files:.pcb=.pdf}

pcb_output_inst=	\
	${pcb_files:.pcb=.png}

pcb_output=	${pcb_output_inst} ${pcb_output_noinst}

# Additional images
images= \
	gcode_control_img.eps \
	gcode_tool_path.eps

images_output_noinst= \
	${images:.eps=.pdf}

images_output_inst= \
	${images:.eps=.png}

images_output= ${images_output_noinst} ${images_output_inst}

${inline_texi} : extracted-docs

# no need to build these when building via a tarball.  They're not used then
# anyway.
.PHONY : extracted-docs
extracted-docs :
	${PERL} $(srcdir)/extract-docs $(srcdir)

SUFFIXES = .eps .pcb .pdf .png .tab .tex .html .texi

.pcb.eps :
	${PCB} -x eps --only-visible --font-path $(top_srcdir)/src --eps-file $@ $<

.pcb.png :
	${PCB} -x png --only-visible --font-path $(top_srcdir)/src --outfile $@ $<

.eps.pdf :
	${PS2PDF} `${AWK} 'BEGIN{s=8}; /BoundingBox/ {printf("-r%d -g%dx%d", s*72, s*$$4, s*$$5);}' $<` $< $@

.tab.texi:
	${ASCII2TEXI} $< > $@

.tex.pdf:
	${PDFLATEX} $<
	${PDFLATEX} $<
	${PDFLATEX} $<

.eps.png:
	${PERL} $(srcdir)/eps2png --png $< > $@

%.html: %.texi
	rm $@ 2>/dev/null; true
	makeinfo --html --css-include=pcb.css --no-split  --output=$@ $^

