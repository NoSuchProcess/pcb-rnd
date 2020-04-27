#!/bin/sh

find . -name '*.[chly]' -exec sed -i "$@" {} \;
find . -name 'config.h.in' -exec sed -i "$@" {} \;

