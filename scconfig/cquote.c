#include <stdio.h>

void copy()
{
	int c, nl = 0, qt = 1, ind = 1;
	while((c = getc(stdin)) != EOF) {
		if (ind) {
			printf("\t");
			if (!nl)
				printf("   ");
			ind = 0;
		}
		if (nl) {
			printf("NL ");
			nl = 0;
		}
		if (qt) {
			printf("\"");
			qt = 0;
		}
		switch(c) {
			case '\t': printf("	");  break;
			case '\n': printf("\"\n"); nl = qt = ind = 1; break;
			case '\r': break;
			case '\\': printf("\\\\");  break;
			case '"': printf("\\\"");  break;
			default:
				if ((c < 32) || (c>126))
					printf("\\%3o", c);
				else
					putc(c, stdout);
		}
	}
	if (!qt)
		printf("\"");
	if (nl) {
		if (ind)
			printf("\t");
		printf("NL");
	}
	printf(";\n");
}



int main(int argc, char *argv[])
{

	printf("#define NL \"\\n\"\n");

	copy();
	return 0;
}
