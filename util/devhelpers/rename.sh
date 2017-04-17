#!/bin/sh

find . -name '*.[chly]' -exec sed -i "$@" {} \;

