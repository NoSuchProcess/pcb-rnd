


	uninit_func = hid_fp_fs_init();
	pcb_plugin_register("fp_fs", "<fp_fs>", NULL, 0, uninit_func);


	uninit_func = hid_fp_wget_init();
	pcb_plugin_register("fp_wget", "<fp_wget>", NULL, 0, uninit_func);

