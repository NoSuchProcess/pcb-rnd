/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2004 Dan McMahill
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef PCB_COMPAT_MISC_H
#define PCB_COMPAT_MISC_H

#include "config.h"

#include <math.h>

#ifndef HAVE_EXPF
float expf(float);
#endif

#ifndef HAVE_LOGF
float logf(float);
#endif

#ifndef HAVE_RANDOM
long random(void);
#endif

const char *get_user_name(void);

#endif /* PCB_COMPAT_MISC_H */
