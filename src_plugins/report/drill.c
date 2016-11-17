/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This module, drill.c, was written and is Copyright (C) 1997 harry eaton
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */
#include "config.h"

#include "data.h"
#include "drill.h"
#include "macro.h"
#include "obj_pinvia.h"

#define STEP_ELEMENT 50
#define STEP_DRILL   30
#define STEP_POINT   100

/*
 * some local prototypes
 */
static void FillDrill(DrillTypePtr, pcb_element_t *, pcb_pin_t *);
static void InitializeDrill(DrillTypePtr, pcb_pin_t *, pcb_element_t *);


static void FillDrill(DrillTypePtr Drill, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_cardinal_t n;
	pcb_element_t **ptr;
	pcb_pin_t ** pin;

	pin = GetDrillPinMemory(Drill);
	*pin = Pin;
	if (Element) {
		Drill->PinCount++;
		for (n = Drill->ElementN - 1; n != -1; n--)
			if (Drill->Element[n] == Element)
				break;
		if (n == -1) {
			ptr = GetDrillElementMemory(Drill);
			*ptr = Element;
		}
	}
	else
		Drill->ViaCount++;
	if (PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin))
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
	drill->Element = NULL;
	drill->PinN = 0;
	drill->Pin = NULL;
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

	AllDrills = (DrillInfoTypePtr) calloc(1, sizeof(DrillInfoType));
	PCB_PIN_ALL_LOOP(top);
	{
		if (!DrillFound) {
			DrillFound = pcb_true;
			Drill = GetDrillInfoDrillMemory(AllDrills);
			InitializeDrill(Drill, pin, element);
		}
		else {
			if (Drill->DrillSize == pin->DrillingHole)
				FillDrill(Drill, element, pin);
			else {
				NewDrill = pcb_false;
				DRILL_LOOP(AllDrills);
				{
					if (drill->DrillSize == pin->DrillingHole) {
						Drill = drill;
						FillDrill(Drill, element, pin);
						break;
					}
					else if (drill->DrillSize > pin->DrillingHole) {
						if (!NewDrill) {
							NewDrill = pcb_true;
							InitializeDrill(&swapdrill, pin, element);
							Drill = GetDrillInfoDrillMemory(AllDrills);
							Drill->DrillSize = pin->DrillingHole + 1;
							Drill = drill;
						}
						savedrill = *drill;
						*drill = swapdrill;
						swapdrill = savedrill;
					}
				}
				PCB_END_LOOP;
				if (AllDrills->Drill[AllDrills->DrillN - 1].DrillSize < pin->DrillingHole) {
					Drill = GetDrillInfoDrillMemory(AllDrills);
					InitializeDrill(Drill, pin, element);
				}
			}
		}
	}
	PCB_ENDALL_LOOP;
	PCB_VIA_LOOP(top);
	{
		if (!DrillFound) {
			DrillFound = pcb_true;
			Drill = GetDrillInfoDrillMemory(AllDrills);
			Drill->DrillSize = via->DrillingHole;
			FillDrill(Drill, NULL, via);
		}
		else {
			if (Drill->DrillSize != via->DrillingHole) {
				DRILL_LOOP(AllDrills);
				{
					if (drill->DrillSize == via->DrillingHole) {
						Drill = drill;
						FillDrill(Drill, NULL, via);
						break;
					}
				}
				PCB_END_LOOP;
				if (Drill->DrillSize != via->DrillingHole) {
					Drill = GetDrillInfoDrillMemory(AllDrills);
					Drill->DrillSize = via->DrillingHole;
					FillDrill(Drill, NULL, via);
				}
			}
			else
				FillDrill(Drill, NULL, via);
		}
	}
	PCB_END_LOOP;
	qsort(AllDrills->Drill, AllDrills->DrillN, sizeof(DrillType), DrillQSort);
	return (AllDrills);
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
				d->Drill[i].Element = (pcb_element_t **) realloc(d->Drill[i].Element, d->Drill[i].ElementMax * sizeof(pcb_element_t *));

				for (ei = 0; ei < d->Drill[i + 1].ElementN; ei++) {
					for (ej = 0; ej < d->Drill[i].ElementN; ej++)
						if (d->Drill[i].Element[ej] == d->Drill[i + 1].Element[ei])
							break;
					if (ej == d->Drill[i].ElementN)
						d->Drill[i].Element[d->Drill[i].ElementN++]
							= d->Drill[i + 1].Element[ei];
				}
			}
			free(d->Drill[i + 1].Element);
			d->Drill[i + 1].Element = NULL;

			d->Drill[i].PinMax = d->Drill[i].PinN + d->Drill[i + 1].PinN;
			d->Drill[i].Pin = (pcb_pin_t **) realloc(d->Drill[i].Pin, d->Drill[i].PinMax * sizeof(pcb_pin_t *));
			memcpy(d->Drill[i].Pin + d->Drill[i].PinN, d->Drill[i + 1].Pin, d->Drill[i + 1].PinN * sizeof(pcb_pin_t *));
			d->Drill[i].PinN += d->Drill[i + 1].PinN;
			free(d->Drill[i + 1].Pin);
			d->Drill[i + 1].Pin = NULL;

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
		free(drill->Element);
		free(drill->Pin);
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
pcb_pin_t ** GetDrillPinMemory(DrillTypePtr Drill)
{
	pcb_pin_t **pin;

	pin = Drill->Pin;

	/* realloc new memory if necessary and clear it */
	if (Drill->PinN >= Drill->PinMax) {
		Drill->PinMax += STEP_POINT;
		pin = (pcb_pin_t **) realloc(pin, Drill->PinMax * sizeof(pcb_pin_t **));
		Drill->Pin = pin;
		memset(pin + Drill->PinN, 0, STEP_POINT * sizeof(pcb_pin_t **));
	}
	return (pin + Drill->PinN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a DrillElement, allocates memory if necessary
 */
pcb_element_t **GetDrillElementMemory(DrillTypePtr Drill)
{
	pcb_element_t **element;

	element = Drill->Element;

	/* realloc new memory if necessary and clear it */
	if (Drill->ElementN >= Drill->ElementMax) {
		Drill->ElementMax += STEP_ELEMENT;
		element = (pcb_element_t **) realloc(element, Drill->ElementMax * sizeof(pcb_element_t **));
		Drill->Element = element;
		memset(element + Drill->ElementN, 0, STEP_ELEMENT * sizeof(pcb_element_t **));
	}
	return (element + Drill->ElementN++);
}

