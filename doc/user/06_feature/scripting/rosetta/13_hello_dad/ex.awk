function hello()
{
	dad("heldi", "new")
	dad("heldi", "begin_vbox")
		dad("heldi", "label", "Hello")
		dad("heldi", "label", "world!")
	dad("heldi", "end")
	dad("heldi", "run_modal", "hello-world", "")
}

BEGIN {
	fgw_func_reg("hello")
}

