function hello();
begin
	message('hello!\n');
end;

function main(ARGV);
begin
	fgw_func_reg(hello);
	cookie := scriptcookie();
	createmenu('/main_menu/Plugins/script/hello', 'hello()', 'Prints hello! in the log', cookie);
end;

