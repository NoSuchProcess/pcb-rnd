#!/bin/sh

# This file is placed in the Public Domain.

# Print the path of source files that may miss the copyright banner among
# with their length in line

find . -name '*.[chly]' -exec sh -c 'grep "^ [*]  Copyright [(]C[)]" {} >/dev/null || wc -l "{}"' \;
