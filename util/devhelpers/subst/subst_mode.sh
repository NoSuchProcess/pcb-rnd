#!/bin/sh
for n in `find . -name "*.[chly]"`
do
sed '
{
	s/RECTANGLE_MODE/PCB_MODE_RECTANGLE/g
	s/POLYGON_MODE/PCB_MODE_POLYGON/g
	s/PASTEBUFFER_MODE/PCB_MODE_PASTE_BUFFER/g
	s/ROTATE_MODE/PCB_MODE_ROTATE/g
	s/REMOVE_MODE/PCB_MODE_REMOVE/g
	s/INSERTPOINT_MODE/PCB_MODE_INSERT_POINT/g
	s/RUBBERBANDMOVE_MODE/PCB_MODE_RUBBERBAND_MOVE/g
	s/THERMAL_MODE/PCB_MODE_THERMAL/g
	s/ARROW_MODE/PCB_MODE_ARROW/g
	s/LOCK_MODE/PCB_MODE_LOCK/g
	s/POLYGONHOLE_MODE/PCB_MODE_POLYGON_HOLE/g
	s/ARC_MODE/PCB_MODE_ARC/g
	s/PAN_MODE/PCB_MODE_PAN/g
	s/MOVE_MODE/PCB_MODE_MOVE/g
	s/COPY_MODE/PCB_MODE_COPY/g
	s/TEXT_MODE/PCB_MODE_TEXT/g
	s/NO_MODE/PCB_MODE_NO/g
	s/VIA_MODE/PCB_MODE_VIA/g
	s/LINE_MODE/PCB_MODE_LINE/g
}
' -i $n
done
