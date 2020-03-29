/* header integrity test: nothing should be included from src/ that is
   not part of librnd */
#include "inc_all.h"

#include "glue.c"

int main(int argc, char *argv[])
{
	return 0;
}
