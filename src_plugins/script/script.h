extern int rnd_script_load(rnd_hidlib_t *hl, const char *id, const char *fn, const char *lang);
extern int rnd_script_unload(const char *id, const char *preunload);
const char *rnd_script_guess_lang(rnd_hidlib_t *hl, const char *fn, int is_filename);

