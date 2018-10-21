function uconv()
	if dad("uconv", "exists") ~= 0 then
		return
	end

	dad("uconv", "new")
	dad("uconv", "begin_hbox")
		inp = dad("uconv", "coord", 0, "100m")
			dad("uconv", "onchange", "uconv_update")
		dad("uconv", "begin_vbox")
			res1 = dad("uconv", "label", "-")
			res2 = dad("uconv", "label", "-")
			res3 = dad("uconv", "label", "-")
		dad("uconv", "end")
	dad("uconv", "end")
	dad("uconv", "run", "Unit converter", "")
end

function uconv_update()
	dad("uconv", "set", res1, dad("uconv", "get", inp, "mm") .. " mm")
	dad("uconv", "set", res2, dad("uconv", "get", inp, "mil") .. " mil")
	dad("uconv", "set", res3, dad("uconv", "get", inp, "inch") .. " inch")
end

fgw_func_reg("uconv")
fgw_func_reg("uconv_update")

