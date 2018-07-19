# An action without arguments that prints hello world in the log window
# Returns 0 for success.
def hello():
	message("Hello world!");
	return 0

# register hello() action
fgw_func_reg("hello")
