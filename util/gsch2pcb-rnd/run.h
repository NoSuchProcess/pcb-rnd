/**
 * Build and run a command. No redirection or error handling is
 * done.  Format string is split on whitespace. Specifiers %l and %s
 * are replaced with contents of positional args. To be recognized,
 * specifiers must be separated from other arguments in the format by
 * whitespace.
 *  - %L expects a gadl_list_t vith char * payload, contents used as separate arguments
 *  - %s expects a char*, contents used as a single argument (omitted if NULL)
 * @param[in] format  used to specify command to be executed
 * @param[in] ...     positional parameters
 */
int build_and_run_command(const char * format_, ...);

