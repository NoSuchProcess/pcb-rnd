/* Instantiate exactly one global lpr HID. The actual plugin is a thing wrapper
   implemented by the app: it needs to glue an export hid (e.g. ps) to lpr. */

/* Call this from pplg_uninit_export_lpr() */
void rnd_lpr_uninit(void);

/* Call this from pplg_init_export_lpr(); glue arguments:
   - ps_hid: target HID used for the export; typically ps
   - ps_ps_init: callback that fills in the draw calls of the HID struct
   - hid_export_to_file: direct call to export the current design
   - [xy]calib: optional, pointing to conf values used as initial/default calib
                fields in the target hid; when NULL, initialize these to 1.0
                (the user can change these later as exporter options)
                (ignored if the target HID doesn't have xcalib/ycalib options)
*/
int rnd_lpr_init(rnd_hid_t *ps_hid, void (*ps_ps_init)(rnd_hid_t *), void (*hid_export_to_file)(FILE *, rnd_hid_attr_val_t *, rnd_xform_t *), const double *xcalib, const double *ycalib);
