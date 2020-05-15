void libporty_net_detect_target()
{
	require("sys/byte_order", 1, 1);
	require("cc/inline", 1, 0);

/* for bgtask */
	require("libs/lpthread", 1, 0);

	require("libs/time/usleep/presents", 1, 0);
	require("libs/time/Sleep/presents", 1, 0);

	/* find a suitable way to query subsecond time - prefer gettimeofday over ftime */
	if (require("libs/time/gettimeofday/presents", 1, 0))
		require("libs/time/ftime/presents", 1, 1);

	/* for P_quick_exit() */
	if (require("cc/_exit/presents", 1, 0)) {
		require("signal/raise/presents", 1, 1);
		require("signal/names/*", 1, 1);
	}


	/* socket library for net */
	require("libs/socket/poll/presents", 1, 0);
	require("libs/socket/select/presents", 1, 0);
	require("libs/socket/closesocket/presents", 1, 0);
	require("libs/vdprintf", 1, 0);
	require("libs/vsnprintf", 1, 0);
	require("libs/snprintf", 1, 0);

	require("libs/socket/socketpair/presents", 1, 0);
	require("libs/socket/ioctl/presents", 1, 0);
	require("libs/socket/ioctl/fionbio/presents", 1, 1); /* fatal because there is no alternative at the moment */
	require("libs/socket/lac/presents", 1, 1);
	require("libs/socket/recvsend/presents", 1, 1);
	require("libs/socket/readwrite/presents", 1, 0);
	require("libs/socket/shutdown/presents", 1, 1);
	require("libs/socket/ntoh/presents", 1, 1);
	if (require("libs/socket/getaddrinfo/presents", 1, 0))
		put("local/net/use_porty_resolver", strue);
	else
		put("local/net/use_porty_resolver", sfalse);


	/* Try to find constants for shutdown - SHUT_* on POSIX and SD_* on win32.
	   If none found we will fall back using hardwired integers. */
	if (require("libs/socket/SHUT/presents", 1, 0))
		require("libs/socket/SD/presents", 1, 0);

	if (require("libs/fs/readdir/presents", 1, 0))
		require("libs/fs/findnextfile/presents", 1, 1);


	/* detect integer types for host/types.h */
		require("sys/types/size/1_u_int", 1, 1);
		require("sys/types/size/2_u_int", 1, 1);
		require("sys/types/size/4_u_int", 1, 1);
		require("sys/types/size/1_s_int", 1, 1);
		require("sys/types/size/2_s_int", 1, 1);
		require("sys/types/size/4_s_int", 1, 1);

		require("sys/ptrwidth", 1, 1);
		require("sys/types/size_t/broken", 1, 0);
		require("sys/types/off_t/broken", 1, 0);
		require("sys/types/ptrdiff_t/broken", 1, 0);
}
