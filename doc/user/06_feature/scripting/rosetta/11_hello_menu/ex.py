def hello():
	message("Hello world!");
	return 0

# register hello() action
fgw_func_reg("hello")
cookie = scriptcookie()
createmenu("/main_menu/Plugins/script/hello", "hello()", "Prints hello! in the log", cookie)
