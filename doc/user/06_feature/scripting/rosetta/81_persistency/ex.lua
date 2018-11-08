function preunload(why)
	message("preunload for " .. why .. "; counter will be " .. ctr)
	return ctr
end

ctr = scriptpersistency("read");
if ctr == -1 then
	ctr = 0;
end
ctr = ctr + 1

message("pers reload: the script has been loaded " .. ctr .. " times so far.")
--	scriptpersistency("remove")

fgw_func_reg("preunload")

