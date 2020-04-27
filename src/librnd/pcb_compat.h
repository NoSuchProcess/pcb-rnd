/* Renames of old pcb_ prefixed (and unprefixed) symbols to the rnd_ prefixed
   variants. Included from librnd/config.h when PCB_REGISTER_ACTIONS is
   #defined.

   This mechanism is used for the transition period before librnd is moved
   out to a new repository. Ringdove apps may use the old names so they
   work with multiple versions of librnd. Once the move-out takes place,
   a new epoch is defined and all ringdove apps must use the rnd_ prefixed
   symbols only (and this file will be removed).
*/
#define pcb_register_action rnd_register_action
#define pcb_register_actions rnd_register_actions
#define PCB_REGISTER_ACTIONS RND_REGISTER_ACTIONS
#define pcb_action_s rnd_action_s
#define pcb_action_t rnd_action_t
#define pcb_fgw rnd_fgw
#define PCB_PTR_DOMAIN_IDPATH RND_PTR_DOMAIN_IDPATH
#define PCB_PTR_DOMAIN_IDPATH_LIST RND_PTR_DOMAIN_IDPATH_LIST
#define pcb_actions_init rnd_actions_init
#define pcb_actions_uninit rnd_actions_uninit
#define pcb_print_actions rnd_print_actions
#define pcb_dump_actions rnd_dump_actions
#define pcb_find_action rnd_find_action
#define pcb_remove_actions rnd_remove_actions
#define pcb_remove_actions_by_cookie rnd_remove_actions_by_cookie
#define pcb_action rnd_action
#define pcb_actionva rnd_actionva
#define pcb_actionv rnd_actionv
#define pcb_actionv_ rnd_actionv_
#define pcb_actionl rnd_actionl
#define pcb_actionv_bin rnd_actionv_bin
#define pcb_parse_command rnd_parse_command
#define pcb_parse_actions rnd_parse_actions
#define pcb_cli_prompt rnd_cli_prompt
#define pcb_cli_enter rnd_cli_enter
#define pcb_cli_leave rnd_cli_leave
#define pcb_cli_tab rnd_cli_tab
#define pcb_cli_edit rnd_cli_edit
#define pcb_cli_mouse rnd_cli_mouse
#define pcb_cli_uninit rnd_cli_uninit
#define pcb_hid_get_coords rnd_hid_get_coords
#define PCB_ACTION_MAX_ARGS RND_ACTION_MAX_ARGS
#define PCB_ACT_HIDLIB RND_ACT_HIDLIB
#define pcb_act_lookup rnd_act_lookup
#define pcb_make_action_name rnd_make_action_name
#define pcb_aname rnd_aname
#define pcb_act_result rnd_act_result
#define PCB_ACT_CALL_C RND_ACT_CALL_C
#define PCB_ACT_CONVARG RND_PCB_ACT_CONVARG
#define PCB_ACT_IRES RND_ACT_IRES
#define PCB_ACT_DRES RND_ACT_DRES
#define PCB_ACT_FAIL RND_ACT_FAIL
#define pcb_fgw_types_e rnd_fgw_types_e
#define pcb_hidlib_t rnd_hidlib_t
#define pcb_coord_t rnd_coord_t
#define pcb_bool rnd_bool
#define pcb_message rnd_message
#define PCB_ACTION_NAME_MAX RND_ACTION_NAME_MAX
#define pcb_attribute_list_s rnd_attribute_list_s
#define pcb_attribute_list_t rnd_attribute_list_t
#define pcb_attribute_t rnd_attribute_t
#define pcb_attribute_s rnd_attribute_s
#define pcb_attribute_get rnd_attribute_get
#define pcb_attribute_get_ptr rnd_attribute_get_ptr
#define pcb_attribute_get_namespace rnd_attribute_get_namespace
#define pcb_attribute_get_namespace_ptr rnd_attribute_get_namespace_ptr
#define pcb_attribute_put rnd_attribute_put
#define pcb_attrib_get rnd_attrib_get
#define pcb_attrib_put rnd_attrib_put
#define pcb_attribute_remove rnd_attribute_remove
#define pcb_attrib_remove rnd_attrib_remove
#define pcb_attribute_remove_idx rnd_attribute_remove_idx
#define pcb_attribute_free rnd_attribute_free
#define pcb_attribute_copy_all rnd_attribute_copy_all
#define pcb_attribute_copyback_begin rnd_attribute_copyback_begin
#define pcb_attribute_copyback rnd_attribute_copyback
#define pcb_attribute_copyback_end rnd_attribute_copyback_end
#define pcb_attrib_compat_set_intconn rnd_attrib_compat_set_intconn
#define base64_write_right rnd_base64_write_right
#define base64_parse_grow rnd_base64_parse_grow
#define pcb_box_list_s rnd_box_list_s
#define pcb_box_list_t rnd_box_list_t
#define PCB_BOX_ROTATE_CW RND_BOX_ROTATE_CW
#define PCB_BOX_ROTATE_TO_NORTH RND_BOX_ROTATE_TO_NORTH
#define PCB_BOX_ROTATE_FROM_NORTH RND_BOX_ROTATE_FROM_NORTH
#define PCB_BOX_CENTER_X RND_BOX_CENTER_X
#define PCB_BOX_CENTER_Y RND_BOX_CENTER_Y
#define PCB_MOVE_POINT RND_MOVE_POINT
#define PCB_BOX_MOVE_LOWLEVEL RND_BOX_MOVE_LOWLEVEL
#define pcb_point_in_box rnd_point_in_box
#define pcb_point_in_closed_box rnd_point_in_closed_box
#define pcb_box_is_good rnd_box_is_good
#define pcb_box_intersect rnd_box_intersect
#define pcb_closest_pcb_point_in_box rnd_closest_pcb_point_in_box
#define pcb_box_in_box rnd_box_in_box
#define pcb_clip_box rnd_clip_box
#define pcb_shrink_box rnd_shrink_box
#define pcb_bloat_box rnd_bloat_box
#define pcb_box_center rnd_box_center
#define pcb_box_corner rnd_box_corner
#define pcb_point_box rnd_point_box
#define pcb_close_box rnd_close_box
#define pcb_dist2_to_box rnd_dist2_to_box
#define pcb_box_bump_box rnd_box_bump_box
#define pcb_box_bump_point rnd_box_bump_point
#define pcb_box_rotate90 rnd_box_rotate90
#define pcb_box_enlarge rnd_box_enlarge
