extern int want_coord_bits;

	/* figure coordinate bits */
static void rnd_hook_detect_coord_bits(void)
{
	int int_bits       = safe_atoi(get("sys/types/size/signed_int")) * 8;
	int long_bits      = safe_atoi(get("sys/types/size/signed_long_int")) * 8;
	int long_long_bits = safe_atoi(get("sys/types/size/signed_long_long_int")) * 8;
	int int64_bits     = safe_atoi(get("sys/types/size/uint64_t")) * 8;
	const char *chosen, *postfix;
	char tmp[64];
	int need_stdint = 0;

	if (want_coord_bits == int_bits)             { postfix="U";   chosen = "int";           }
	else if (want_coord_bits == long_bits)       { postfix="UL";  chosen = "long int";      }
	else if (want_coord_bits == int64_bits)      { postfix="ULL"; chosen = "int64_t";       need_stdint = 1; }
	else if (want_coord_bits == long_long_bits)  { postfix="ULL"; chosen = "long long int"; }
	else {
		report("ERROR: can't find a suitable integer type for coord to be %d bits wide\n", want_coord_bits);
		exit(1);
	}

	sprintf(tmp, "((1%s<<%d)-1)", postfix, want_coord_bits - 1);
	put("/local/pcb/coord_type", chosen);
	put("/local/pcb/coord_max", tmp);

	chosen = NULL;
	if (istrue(get("/local/pcb/debug"))) { /* debug: c89 */
		if (int64_bits >= 64) {
			/* to suppress warnings on systems that support c99 but are forced to compile in c89 mode */
			chosen = "int64_t";
			need_stdint = 1;
		}
	}

	if (chosen == NULL) { /* non-debug, thus non-c89 */
		if (long_long_bits >= 64) chosen = "long long int";
		else if (long_bits >= 64) chosen = "long int";
		else chosen = "double";
	}
	put("/local/pcb/long64", chosen);
	if (need_stdint)
		put("/local/pcb/include_stdint", "#include <stdint.h>");
}
