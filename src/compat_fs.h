char *GetWorkingDirectory(char *);

int pcb_mkdir(const char *path, int mode);
int pcb_spawnvp(char **argv);
char *tempfile_name_new(char *name);
int tempfile_unlink(char *name);
