#include "layout.h"
#include "src/board.h"
#include "src/change.h"

int layout_get_page_width()
{
	return PCB->MaxWidth;
}

int layout_get_page_height()
{
	return PCB->MaxHeight;
}

void layout_set_page_size(int width, int height)
{
	pcb_board_resize(MIN(PCB_MAX_COORD, MAX(width, PCB_MIN_SIZE)), MIN(PCB_MAX_COORD, MAX(height, PCB_MIN_SIZE)));
}

