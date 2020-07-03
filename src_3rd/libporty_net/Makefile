ROOT=../..

#CFLAGS = -Wall -O3

libportytcp4.o: libportytcp4.c libportytcp4.h

libportytcp4.c libportytcp4.h: concat.sh
	./concat.sh

include ../../Makefile.conf

clean:

distclean:
	$(SCCBOX) rm -f -q os_includes.h pnet_config.h phost_types.h