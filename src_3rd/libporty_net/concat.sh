#!/bin/sh

tmp=net

fixinc()
{
	grep -v "^#include \"\|^#warning"
}

svn checkout svn://repo.hu/libporty/trunk/src/libporty/net $tmp
svn cat svn://repo.hu/libporty/trunk/src/libporty/config.h.in > pnet_config.h.in
cp net/os_includes.h.in .

(echo '#include "os_includes.h"'
cat $tmp/network.h $tmp/os_dep.h $tmp/tcp4.h $tmp/dns4.h $tmp/uninit_chain.h | fixinc) > libportytcp4.h

(echo '#include "libportytcp4.h"'
cat $tmp/os_dep.c $tmp/tcp4.c $tmp/dns4.c $tmp/uninit_chain.c | fixinc) > libportytcp4.c


