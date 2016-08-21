#include <stdio.h>
#include "uniq_name.h"

int main()
{
	unm_t group1;

	/* Initialize the group with defaults */
	unm_init(&group1);

	printf("'%s'\n", unm_name(&group1, "foo", (void *)0x01));
	printf("'%s'\n", unm_name(&group1, "bar", (void *)0x42));
	printf("'%s'\n", unm_name(&group1, "bar", (void *)0x66));
	printf("'%s'\n", unm_name(&group1, "baz", NULL));

	/* test forced collision */
	printf("'%s'\n", unm_name(&group1, "forced", NULL));
	printf("'%s'\n", unm_name(&group1, "forced_dup1", NULL));
	printf("'%s'\n", unm_name(&group1, "forced_dup2", NULL));
	printf("'%s'\n", unm_name(&group1, "forced", NULL)); /* this would end up as forced_dup3 */
	printf("'%s'\n", unm_name(&group1, "forced", NULL)); /* ... _dup4 */

	/* test global dup ctr */
	printf("'%s'\n", unm_name(&group1, "foo", NULL)); /* ... _dup5 */

	/* unknown name gets named */
	printf("'%s'\n", unm_name(&group1, NULL, NULL)); /* this is the first unnamed item, so it won't get a suffix */
	printf("'%s'\n", unm_name(&group1, NULL, NULL)); /* gets suffixed because of the collision */

	/* empty name is like unknown name too */
	printf("'%s'\n", unm_name(&group1, "", NULL));


	/* Optional: change configuration, even on the fly: */
	group1.unnamed = "anonymous";
	group1.suffix_sep = "::";

	/* Note: it is safe to reset the counter - worst case the collision loop
	   will run some more before it finds a free name, but there won't be
	   a collision */
	group1.ctr = 0;

	/* Adding three more unnamed items will demonstrate how the new config settings take effect */
	printf("'%s'\n", unm_name(&group1, NULL, NULL));
	printf("'%s'\n", unm_name(&group1, NULL, NULL));
	printf("'%s'\n", unm_name(&group1, NULL, NULL));

	/* Optional: iterate over all items once more, e.g. to generate an index
	   Warning: order is not guaranteed. */
	{
		htsp_entry_t *e;
		for (e = htsp_first(&group1.seen); e; e = htsp_next(&group1.seen, e))
			printf("name='%s' user_data='%p'\n", e->key, e->value);
	}

	/* Release all memory */
	unm_uninit(&group1);
}
