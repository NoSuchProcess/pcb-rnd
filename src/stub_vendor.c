/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include "stub_vendor.h"

static int stub_vendorDrillMap_dummy(int dia)
{
	return dia;
}

static pcb_bool stub_vendorIsElementMappable_dummy(pcb_element_t *e)
{
	return pcb_false;
}

int (*pcb_stub_vendor_drill_map)(int) = stub_vendorDrillMap_dummy;
pcb_bool (*pcb_stub_vendor_is_element_mappable)(pcb_element_t *) = stub_vendorIsElementMappable_dummy;


