function preunload(why)
{
	message("preunload for " why "; counter will be " ctr "\n")
	return ctr
}

BEGIN {
	ctr = scriptpersistency("read");
	if (ctr == -1)
		ctr = 0;
	ctr++
	message("pers reload: the script has been loaded " ctr " times so far.\n")
#	scriptpersistency("remove")

	fgw_func_reg("preunload")
}
