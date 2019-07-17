{ An action without arguments that prints hello world in the log window
  Returns 0 for success.}
function hello();
begin
	message('Hello world!\n');
	hello := 0;
end;

{ Register the action - action name matches the function name }
function main(ARG);
begin
	fgw_func_reg(hello);
end;

