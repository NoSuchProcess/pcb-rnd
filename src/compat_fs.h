char *pcb_get_wd(char *);

int pcb_mkdir(const char *path, int mode);
int pcb_spawnvp(const char **argv);

/* Return 1 if path is a file that can be opened for read */
int pcb_file_readable(const char *path);

char *pcb_tempfile_name_new(const char *name);

/* remove temporary file and _also_ free the memory for name
 * (this fact is a little confusing)
 */
int pcb_tempfile_unlink(char *name);

/* Return non-zero if path is a directory */
int pcb_is_dir(const char *path);

/* Return non-zero if fn is an absolute path */
int pcb_is_path_abs(const char *fn);
