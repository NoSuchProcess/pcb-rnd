function timer_cb(now, remain, ud)
	message("timer_cb: " @ now @ " " @ remain @ " " @ ud @ "\n")
	timer_cb = 0
end function


fgw_func_reg(timer_cb)

addtimer("timer_cb", 0, 1, "single, async")
addtimer("timer_cb", 1.2, 1, "single, delayed")
addtimer("timer_cb", 0.5, 3, "multi")

message("Script init finished!\n")
