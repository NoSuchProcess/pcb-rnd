function hello()
{
	message("hello!\n")
}

BEGIN {
	fgw_func_reg("hello")
	cookie = scriptcookie()
	createmenu("/main_menu/Plugins/script/hello", "hello()", "Prints hello! in the log", cookie)
}
