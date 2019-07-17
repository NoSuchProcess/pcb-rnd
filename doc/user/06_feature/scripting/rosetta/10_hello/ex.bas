REM An action without arguments that prints hello world in the log window
REM Returns 0 for success.
function hello()
	message("Hello world!\n")
	hello = 0
end function

REM Register the action - action name matches the function name
fgw_func_reg(hello)


