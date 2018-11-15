function hello()
	message("Hello world!")
	return 0
end

fgw_func_reg("hello")
cookie = scriptcookie()
createmenu("/main_menu/Plugins/script/hello", "hello()", "Prints hello! in the log", cookie)
