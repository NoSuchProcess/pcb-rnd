# An action without arguments that prints hello world in the log window
# Returns 0 for success.
def hello()
	message("Hello world!\n")
	return 0
end

# Register the action - action name matches the function name
fgw_func_reg("hello")
