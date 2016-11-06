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
	ChangePCBSize (MIN(MAX_COORD, MAX(width, MIN_SIZE)), MIN(MAX_COORD, MAX(height, MIN_SIZE)));
}

