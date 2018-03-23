/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, drill.c, was written and is Copyright (C) 1997 harry eaton
 *  and upgraded by Tibor 'Igor2' Palinkas for padstacks in 2017
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include "config.h"

#include "data.h"
#include "drill.h"
#include "macro.h"
#include "obj_pinvia.h"
#include "obj_pstk_inlines.h"
#include "obj_elem.h"

#define STEP_ELEMENT 50
#define STEP_DRILL   30
#define STEP_POINT   100

static void InitializeDrill(DrillTypePtr, pcb_pin_t *, pcb_element_t *);

static void FillDrill(DrillTypePtr Drill, pcb_any_obj_t *hole, int unplated)
{
	pcb_cardinal_t n;
	pcb_any_obj_t **pnew;
	pcb_any_obj_t **hnew;

	hnew = GetDrillPinMemory(Drill);
	*hnew = hole;
	if ((hole->parent_type == PCB_PARENT_ELEMENT) || (hole->parent_type == PCB_PARENT_SUBC)) {
		Drill->PinCount++;
		for (n = Drill->ElementN - 1; n != -1; n--)
			if (Drill->parent[n] == hole->parent.any)
				break;
		if (n == -1) {
			pnew = GetDrillElementMemory(Drill);
			*pnew = hole->parent.any;
		}
	}
	else
		Drill->ViaCount++;
	if (unplated)
		Drill->UnplatedCount++;
}

static void InitializeDrill(DrillTypePtr drill, pcb_pin_t *pin, pcb_element_t *element)
{
	void *ptr;

	drill->DrillSize = pin->DrillingHole;
	drill->ElementN = 0;
	drill->ViaCount = 0;
	drill->PinCount = 0;
	drill->UnplatedCount = 0;
	drill->ElementMax = 0;
	drill->parent = NULL;
	drill->PinN = 0;
	drill->hole = NULL;
	drill->PinMax = 0;
	ptr = (void *) GetDrillPinMemory(drill);
	*((pcb_pin_t **) ptr) = pin;
	if (element) {
		ptr = (void *) GetDrillElementMemory(drill);
		*((pcb_element_t **) ptr) = element;
		drill->PinCount = 1;
	}
	else
		drill->ViaCount = 1;
	if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin))
		drill->UnplatedCount = 1;
}

static int DrillQSort(const void *va, const void *vb)
{
	DrillType *a = (DrillType *) va;
	DrillType *b = (DrillType *) vb;
	return a->DrillSize - b->DrillSize;
}

DrillInfoTypePtr GetDrillInfo(pcb_data_t *top)
{
	DrillInfoTypePtr AllDrills;
	DrillTypePtr Drill = NULL;
	DrillType savedrill, swapdrill;
	pcb_bool DrillFound = pcb_false;
	pcb_bool NewDrill;
	pcb_rtree_it_t it;
	pcb_box_t *pb;

	AllDrills = (DrillInfoTypePtr) calloc(1, sizeof(DrillInfoType));

	for(pb = pcb_r_first(top->padstack_tree, &it); pb != NULL; pb = pcb_r_next(&it)) {
		pcb_pstk_t *ps = (pcb_pstk_t *)pb;
		pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
		if (proto->hdia <= 0)
			continue;
		if (!DrillFound) {
			DrillFound = pcb_true;
			Drill = GetDrillInfoDrillMemory(AllDrills);
			Drill->DrillSize = proto->hdia;
			FillDrill(Drill, (pcb_any_obj_t *)ps, !proto->hplated);
		}
		else {
			if (Drill->DrillSize != proto->hdia) {
				DRILL_LOOP(AllDrills);
				{
					if (drill->DrillSize == proto->hdia) {
						Drill = drill;
						FillDrill(Drill, (pcb_any_obj_t *)ps, !proto->hplated);
						break;
					}
				}
				PCB_END_LOOP;
				if (Drill->DrillSize != proto->hdia) {
					Drill = GetDrillInfoDrillMemory(AllDrills);
					Drill->DrillSize = proto->hdia;
					FillDrill(Drill, (pcb_any_obj_t *)ps, !proto->hplated);
				}
			}
			else
				FillDrill(Drill, (pcb_any_obj_t *)ps, !proto->hplated);
		}
	}
	pcb_r_end(&it);

	qsort(AllDrills->Drill, AllDrills->DrillN, sizeof(DrillType), DrillQSort);
	return AllDrills;
}

#define ROUND(x,n) ((int)(((x)+(n)/2)/(n))*(n))

void RoundDrillInfo(DrillInfoTypePtr d, int roundto)
{
	unsigned int i = 0;

	while ((d->DrillN > 0) && (i < d->DrillN - 1)) {
		int diam1 = ROUND(d->Drill[i].DrillSize, roundto);
		int diam2 = ROUND(d->Drill[i + 1].DrillSize, roundto);

		if (diam1 == diam2) {
			int ei, ej;

			d->Drill[i].ElementMax = d->Drill[i].ElementN + d->Drill[i + 1].ElementN;
			if (d->Drill[i].ElementMax) {
				d->Drill[i].parent = realloc(d->Drill[i].parent, d->Drill[i].ElementMax * sizeof(pcb_any_obj_t *));

				for (ei = 0; ei < d->Drill[i + 1].ElementN; ei++) {
					for (ej = 0; ej < d->Drill[i].ElementN; ej++)
						if (d->Drill[i].parent[ej] == d->Drill[i + 1].parent[ei])
							break;
					if (ej == d->Drill[i].ElementN)
						d->Drill[i].parent[d->Drill[i].ElementN++]
							= d->Drill[i + 1].parent[ei];
				}
			}
			free(d->Drill[i + 1].parent);
			d->Drill[i + 1].parent = NULL;

			d->Drill[i].PinMax = d->Drill[i].PinN + d->Drill[i + 1].PinN;
			d->Drill[i].hole = realloc(d->Drill[i].hole, d->Drill[i].PinMax * sizeof(pcb_any_obj_t *));
			memcpy(d->Drill[i].hole + d->Drill[i].PinN, d->Drill[i + 1].hole, d->Drill[i + 1].PinN * sizeof(pcb_any_obj_t *));
			d->Drill[i].PinN += d->Drill[i + 1].PinN;
			free(d->Drill[i + 1].hole);
			d->Drill[i + 1].hole = NULL;

			d->Drill[i].PinCount += d->Drill[i + 1].PinCount;
			d->Drill[i].ViaCount += d->Drill[i + 1].ViaCount;
			d->Drill[i].UnplatedCount += d->Drill[i + 1].UnplatedCount;

			d->Drill[i].DrillSize = diam1;

			memmove(d->Drill + i + 1, d->Drill + i + 2, (d->DrillN - i - 2) * sizeof(DrillType));
			d->DrillN--;
		}
		else {
			d->Drill[i].DrillSize = diam1;
			i++;
		}
	}
}

void FreeDrillInfo(DrillInfoTypePtr Drills)
{
	DRILL_LOOP(Drills);
	{
		free(drill->parent);
		free(drill->hole);
	}
	PCB_END_LOOP;
	free(Drills->Drill);
	free(Drills);
}

/* ---------------------------------------------------------------------------
 * get next slot for a Drill, allocates memory if necessary
 */
DrillTypePtr GetDrillInfoDrillMemory(DrillInfoTypePtr DrillInfo)
{
	DrillTypePtr drill = DrillInfo->Drill;

	/* realloc new memory if necessary and clear it */
	if (DrillInfo->DrillN >= DrillInfo->DrillMax) {
		DrillInfo->DrillMax += STEP_DRILL;
		drill = (DrillTypePtr) realloc(drill, DrillInfo->DrillMax * sizeof(DrillType));
		DrillInfo->Drill = drill;
		memset(drill + DrillInfo->DrillN, 0, STEP_DRILL * sizeof(DrillType));
	}
	return (drill + DrillInfo->DrillN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a DrillPoint, allocates memory if necessary
 */
pcb_any_obj_t **GetDrillPinMemory(DrillTypePtr Drill)
{
	pcb_any_obj_t **pin;

	pin = Drill->hole;

	/* realloc new memory if necessary and clear it */
	if (Drill->PinN >= Drill->PinMax) {
		Drill->PinMax += STEP_POINT;
		pin = realloc(pin, Drill->PinMax * sizeof(pcb_any_obj_t **));
		Drill->hole = pin;
		memset(pin + Drill->PinN, 0, STEP_POINT * sizeof(pcb_any_obj_t **));
	}
	return (pin + Drill->PinN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a DrillElement, allocates memory if necessary
 */
pcb_any_obj_t **GetDrillElementMemory(DrillTypePtr Drill)
{
	pcb_any_obj_t **element;

	element = Drill->parent;

	/* realloc new memory if necessary and clear it */
	if (Drill->ElementN >= Drill->ElementMax) {
		Drill->ElementMax += STEP_ELEMENT;
		element = realloc(element, Drill->ElementMax * sizeof(pcb_any_obj_t **));
		Drill->parent = element;
		memset(element + Drill->ElementN, 0, STEP_ELEMENT * sizeof(pcb_element_t **));
	}
	return (element + Drill->ElementN++);
}

