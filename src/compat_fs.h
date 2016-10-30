char *GetWorkingDirectory(char *);

int pcb_mkdir(const char *path, int mode);
int pcb_spawnvp(const char **argv);

/* Return 1 if path is a file that can be opened for read */
int pcb_file_readable(const char *path);

char *tempfile_name_new(const char *name);

/* remove temporary file and _also_ free the memory for name
 * (this fact is a little confusing)
 */
int tempfile_unlink(char *name);
