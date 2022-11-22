#!/bin/sh

APP=pcb-rnd

dump_actions() {
	cd ../../../../src
	./pcb-rnd --dump-actions
}

. $LIBRND_LIBDIR/action_compiler.sh
