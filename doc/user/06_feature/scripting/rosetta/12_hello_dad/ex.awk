function hello()
{
	dad("heldi", "new")
	dad("heldi", "label", "Hello world!")
	dad("heldi", "run_modal", "hello-world", "Hello world dialog")
}

BEGIN {
	fgw_func_reg("hello")
}

