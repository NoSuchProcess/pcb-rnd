# An action without arguments that prints hello world in the log window
function hello()
{
	message("Hello world!\n");
}

# Register the action - action name matches the function name
BEGIN {
	fgw_func_reg("hello")
}
