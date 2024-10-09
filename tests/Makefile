test:
	cd RTT && $(MAKE) all && $(MAKE) test
	cd pcbflags && $(MAKE) all && $(MAKE) test
	cd strflags && $(MAKE) all && $(MAKE) test
	cd uniq_name && $(MAKE) all && $(MAKE) test
	cd propedit && $(MAKE) all && $(MAKE) test
	cd drc_pstk_hbrk && $(MAKE) all && $(MAKE) test
	cd drc_query && $(MAKE) test
	cd query && $(MAKE) test
	cd menu && $(MAKE) test
	cd cam && $(MAKE) test
	cd vendordrill && $(MAKE) test
	cd pstk_crescent && $(MAKE) test
	@echo " "
	@echo "+-------------------------------------------------+"
	@echo "+  All tests passed, pcb-rnd is safe to install.  +"
	@echo "+-------------------------------------------------+"

clean:
	cd RTT && $(MAKE) clean
	cd pcbflags && $(MAKE) clean
	cd strflags && $(MAKE) clean
	cd uniq_name && $(MAKE) clean
	cd propedit && $(MAKE) clean
	cd drc_pstk_hbrk && $(MAKE) clean
	cd drc_query && $(MAKE) clean
	cd query && $(MAKE) clean
	cd menu && $(MAKE) clean
	cd cam && $(MAKE) clean
	cd vendordrill && $(MAKE) clean
	cd pstk_crescent && $(MAKE) clean

