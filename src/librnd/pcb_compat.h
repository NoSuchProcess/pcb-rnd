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
#define pcb_box_list_s rnd_rnd_box_list_s
#define pcb_box_list_t rnd_rnd_box_list_t
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
#define pcb_closest_pcb_point_in_box rnd_closest_cheap_point_in_box
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
#define PCB_NORTH RND_NORTH
#define PCB_EAST RND_EAST
#define PCB_SOUTH RND_SOUTH
#define PCB_WEST RND_WEST
#define PCB_NE RND_NE
#define PCB_SE RND_SE
#define PCB_SW RND_SW
#define PCB_NW RND_NW
#define PCB_ANY_DIR RND_ANY_DIR
#define rnd_rnd_box_t rnd_rnd_box_t
#define pcb_direction_t rnd_direction_t
#define pcb_cheap_point_s rnd_cheap_point_s
#define pcb_cheap_point_t rnd_cheap_point_t
#define pcb_distance rnd_distance
#define pcb_cardinal_t rnd_cardinal_t
#define PCB_INLINE RND_INLINE
#define PCB_HAVE_SETENV RND_HAVE_SETENV
#define PCB_HAVE_PUTENV RND_HAVE_PUTENV
#define PCB_HAVE_USLEEP RND_HAVE_USLEEP
#define PCB_HAVE_WSLEEP RND_HAVE_WSLEEP
#define PCB_HAVE_SELECT RND_HAVE_SELECT
#define HAVE_SNPRINTF RND_HAVE_SNPRINTF
#define HAVE_VSNPRINTF RND_HAVE_VSNPRINTF
#define HAVE_GETCWD RND_HAVE_GETCWD
#define HAVE__GETCWD RND_HAVE__GETCWD
#define HAVE_GETWD RND_HAVE_GETWD
#define HAVE_GD_H RND_HAVE_GD_H
#define HAVE_GDIMAGEGIF RND_HAVE_GDIMAGEGIF
#define HAVE_GDIMAGEJPEG RND_HAVE_GDIMAGEJPEG
#define HAVE_GDIMAGEPNG RND_HAVE_GDIMAGEPNG
#define HAVE_GD_RESOLUTION RND_HAVE_GD_RESOLUTION
#define HAVE_GETPWUID RND_HAVE_GETPWUID
#define HAVE_RINT RND_HAVE_RINT
#define HAVE_ROUND RND_HAVE_ROUND
#define WRAP_S_ISLNK RND_WRAP_S_ISLNK
#define HAVE_XINERAMA RND_HAVE_XINERAMA
#define HAVE_XRENDER RND_HAVE_XRENDER
#define USE_LOADLIBRARY RND_USE_LOADLIBRARY
#define HAVE_MKDTEMP RND_HAVE_MKDTEMP
#define HAVE_REALPATH RND_HAVE_REALPATH
#define USE_FORK_WAIT RND_USE_FORK_WAIT
#define USE_SPAWNVP RND_USE_SPAWNVP
#define USE_MKDIR RND_USE_MKDIR
#define USE__MKDIR RND_USE__MKDIR
#define MKDIR_NUM_ARGS RND_MKDIR_NUM_ARGS
#define PCB_HAVE_SIGPIPE RND_HAVE_SIGPIPE
#define PCB_HAVE_SIGSEGV RND_HAVE_SIGSEGV
#define PCB_HAVE_SIGABRT RND_HAVE_SIGABRT
#define PCB_HAVE_SIGINT RND_HAVE_SIGINT
#define PCB_HAVE_SIGHUP RND_HAVE_SIGHUP
#define PCB_HAVE_SIGTERM RND_HAVE_SIGTERM
#define PCB_HAVE_SIGQUIT RND_HAVE_SIGQUIT
#define PCB_HAVE_SYS_FUNGW RND_HAVE_SYS_FUNGW
#define PCB_DIR_SEPARATOR_C RND_DIR_SEPARATOR_C
#define PCB_DIR_SEPARATOR_S RND_DIR_SEPARATOR_S
#define PCB_PATH_DELIMETER RND_PATH_DELIMETER
#define HAS_ATEXIT RND_HAS_ATEXIT
#define HAVE_UNISTD_H RND_HAVE_UNISTD_H
#define COORD_MAX RND_COORD_MAX
#define PCB_FUNC_UNUSED RND_FUNC_UNUSED
#define pcb_color_s rnd_color_s
#define pcb_color_t rnd_color_t
#define pcb_color_black rnd_color_black
#define pcb_color_white rnd_color_white
#define pcb_color_cyan rnd_color_cyan
#define pcb_color_red rnd_color_red
#define pcb_color_blue rnd_color_blue
#define pcb_color_grey33 rnd_color_grey33
#define pcb_color_magenta rnd_color_magenta
#define pcb_color_golden rnd_color_golden
#define pcb_color_drill rnd_color_drill
#define pcb_color_load_int rnd_color_load_int
#define pcb_color_load_packed rnd_color_load_packed
#define pcb_color_load_float rnd_color_load_float
#define pcb_color_load_str rnd_color_load_str
#define pcb_clrdup rnd_clrdup
#define pcb_color_init rnd_color_init
#define pcb_color_is_drill rnd_color_is_drill
#define pcb_clrcache_s rnd_clrcache_s
#define pcb_clrcache_init rnd_clrcache_init
#define pcb_clrcache_del rnd_clrcache_del
#define pcb_clrcache_get rnd_clrcache_get
#define pcb_clrcache_uninit rnd_clrcache_uninit
#define pcb_clrcache_t rnd_clrcache_t
#define pcb_clrcache_free_t rnd_clrcache_free_t
#define PCB_BOOLEAN_EXPR RND_BOOLEAN_EXPR
#define PCB_LIKELY RND_LIKELY
#define PCB_UNLIKELY RND_UNLIKELY
#define pcb_get_wd rnd_get_wd
#define pcb_spawnvp rnd_spawnvp
#define pcb_file_readable rnd_file_readable
#define pcb_tempfile_name_new rnd_tempfile_name_new
#define pcb_tempfile_unlink rnd_tempfile_unlink
#define pcb_is_path_abs rnd_is_path_abs
#define pcb_lrealpath rnd_lrealpath
#define pcb_rand rnd_rand
#define pcb_get_user_name rnd_get_user_name
#define pcb_getpid rnd_getpid
#define pcb_strndup rnd_strndup
#define pcb_strdup rnd_strdup
#define pcb_strdup_null rnd_strdup_null
#define pcb_round rnd_round
#define pcb_strcasecmp rnd_strcasecmp
#define pcb_strncasecmp rnd_strncasecmp
#define pcb_setenv rnd_setenv
#define pcb_print_utc rnd_print_utc
#define pcb_ms_sleep rnd_ms_sleep
#define pcb_ltime rnd_ltime
#define pcb_dtime rnd_dtime
#define pcb_fileno rnd_fileno
#define confitem_t rnd_confitem_t
#define confitem_u rnd_confitem_u
#define pcb_conf_rev rnd_conf_rev
#define conf_policy_t rnd_conf_policy_t
#define POL_PREPEND RND_POL_PREPEND
#define POL_APPEND RND_POL_APPEND
#define POL_OVERWRITE RND_POL_OVERWRITE
#define POL_DISABLE RND_POL_DISABLE
#define POL_invalid RND_POL_invalid
#define conf_flag_t rnd_conf_flag_t
#define CFF_USAGE RND_CFF_USAGE
#define CFT_STRING RND_CFT_STRING
#define CFT_BOOLEAN RND_CFT_BOOLEAN
#define CFT_INTEGER RND_CFT_INTEGER
#define CFT_REAL RND_CFT_REAL
#define CFT_COORD RND_CFT_COORD
#define CFT_UNIT RND_CFT_UNIT
#define CFT_COLOR RND_CFT_COLOR
#define CFT_LIST RND_CFT_LIST
#define CFT_HLIST RND_CFT_HLIST
#define CFN_STRING RND_CFN_STRING
#define CFN_BOOLEAN RND_CFN_BOOLEAN
#define CFN_INTEGER RND_CFN_INTEGER
#define CFN_REAL RND_CFN_REAL
#define CFN_COORD RND_CFN_COORD
#define CFN_UNIT RND_CFN_UNIT
#define CFN_COLOR RND_CFN_COLOR
#define CFN_LIST RND_CFN_LIST
#define CFN_HLIST RND_CFN_HLIST
#define CFN_max RND_CFN_max
#define conf_native_type_t rnd_conf_native_type_t
#define confprop_t rnd_confprop_t
#define conf_native_t rnd_conf_native_t
#define conf_role_t rnd_conf_role_t
#define CFR_INTERNAL RND_CFR_INTERNAL
#define CFR_SYSTEM RND_CFR_SYSTEM
#define CFR_DEFAULTPCB RND_CFR_DEFAULTPCB
#define CFR_USER RND_CFR_USER
#define CFR_ENV RND_CFR_ENV
#define CFR_PROJECT RND_CFR_PROJECT
#define CFR_DESIGN RND_CFR_DESIGN
#define CFR_CLI RND_CFR_CLI
#define CFR_max_real RND_CFR_max_real
#define CFR_file RND_CFR_file
#define CFR_binary RND_CFR_binary
#define CFR_max_alloc RND_CFR_max_alloc
#define CFR_invalid RND_CFR_invalid
#define pcb_conf_default_prio rnd_conf_default_prio
#define pcb_conf_main_root_replace_cnt rnd_conf_main_root_replace_cnt
#define pcb_conf_init rnd_conf_init
#define pcb_conf_uninit rnd_conf_uninit
#define pcb_conf_load_all rnd_conf_load_all
#define pcb_conf_load_as rnd_conf_load_as
#define pcb_conf_insert_tree_as rnd_conf_insert_tree_as
#define pcb_conf_load_project rnd_conf_load_project
#define pcb_conf_load_extra rnd_conf_load_extra
#define pcb_conf_load_extra rnd_conf_load_extra
#define pcb_conf_get_field rnd_conf_get_field
#define pcb_conf_reg_field_ rnd_conf_reg_field_
#define pcb_conf_unreg_field rnd_conf_unreg_field
#define pcb_conf_unreg_fields rnd_conf_unreg_fields
#define pcb_conf_set rnd_conf_set
#define pcb_conf_del rnd_conf_del
#define pcb_conf_grow rnd_conf_grow
#define pcb_conf_set_dry rnd_conf_set_dry
#define pcb_conf_set_native rnd_conf_set_native
#define pcb_conf_set_from_cli rnd_conf_set_from_cli
#define pcb_conf_parse_arguments rnd_conf_parse_arguments
#define pcb_conf_reg_field_array rnd_conf_reg_field_array
#define pcb_conf_reg_field_scalar rnd_conf_reg_field_scalar
#define pcb_conf_reg_field rnd_conf_reg_field
#define pcb_conf_native_type_parse rnd_conf_native_type_parse
#define pcb_conf_policy_parse rnd_conf_policy_parse
#define pcb_conf_policy_name rnd_conf_policy_name
#define pcb_conf_role_parse rnd_conf_role_parse
#define pcb_conf_role_name rnd_conf_role_name
#define pcb_conf_lock rnd_conf_lock
#define pcb_conf_unlock rnd_conf_unlock
#define pcb_conf_replace_subtree rnd_conf_replace_subtree
#define pcb_conf_reset rnd_conf_reset
#define pcb_conf_save_file rnd_conf_save_file
#define pcb_conf_islocked rnd_conf_islocked
#define pcb_conf_isdirty rnd_conf_isdirty
#define pcb_conf_makedirty rnd_conf_makedirty
#define pcb_conf_print_native_field rnd_conf_print_native_field
#define pcb_conf_print_native rnd_conf_print_native
#define pcb_conf_ro rnd_conf_ro
#define pcb_conf_in_production rnd_conf_in_production
#define pcb_conf_update rnd_conf_update
#define pcb_conf_fields rnd_conf_fields
#define conf_pfn rnd_conf_pfn
#define pcb_conf_setf rnd_conf_setf
#define conf_list_foreach_path_first rnd_conf_list_foreach_path_first
#define conf_fields_foreach rnd_conf_fields_foreach
#define conf_set_design rnd_conf_set_design
#define conf_set_editor rnd_conf_set_editor
#define conf_set_editor_ rnd_conf_set_editor_
#define conf_toggle_editor rnd_conf_toggle_editor
#define conf_toggle_heditor rnd_conf_toggle_heditor
#define conf_toggle_editor_ rnd_conf_toggle_editor_
#define conf_toggle_heditor_ rnd_conf_toggle_heditor_
#define conf_force_set_bool rnd_conf_force_set_bool
#define conf_force_set_str rnd_conf_force_set_str
#define pcb_conf_lht_get_first rnd_conf_lht_get_first
#define pcb_conf_list_first_str rnd_conf_list_first_str
#define pcb_conf_list_next_str rnd_conf_list_next_str
#define conf_loop_list rnd_conf_loop_list
#define conf_loop_list_str rnd_conf_loop_list_str
#define pcb_conf_concat_strlist rnd_conf_concat_strlist
#define pcb_conf_usage rnd_conf_usage
#define pcb_conf_lookup_role rnd_conf_lookup_role
#define pcb_conf_lht_get_at rnd_conf_lht_get_at
#define pcb_conf_lht_get_at_mainplug rnd_conf_lht_get_at_mainplug
#define pcb_conf_export_to_file rnd_conf_export_to_file
#define pcb_conf_get_policy_prio rnd_conf_get_policy_prio
#define pcb_conf_parse_text rnd_conf_parse_text
#define pcb_conf_get_user_conf_name rnd_conf_get_user_conf_name
#define pcb_conf_get_project_conf_name rnd_conf_get_project_conf_name
#define pcb_conf_lht_get_first_pol rnd_conf_lht_get_first_pol
#define pcb_conf_reg_file rnd_conf_reg_file
#define pcb_conf_unreg_file rnd_conf_unreg_file
#define pcb_conf_files_uninit rnd_conf_files_uninit
#define pcb_conf_core_postproc rnd_conf_core_postproc
#define pcb_conf_resolve_t rnd_conf_resolve_t
#define pcb_conf_resolve_all rnd_conf_resolve_all
#define pcb_conf_resolve rnd_conf_resolve
#define pcb_conflist_t rnd_conflist_t
#define pcb_conflist_u rnd_conflist_u
#define pcb_conf_listitem_t rnd_conf_listitem_t
#define pcb_conflist_foreach rnd_conflist_foreach
#define pcb_conflist_first rnd_conflist_first
#define pcb_conflist_next rnd_conflist_next
#define pcb_conflist_last rnd_conflist_last
#define pcb_conflist_prev rnd_conflist_prev
#define pcb_conflist_length rnd_conflist_length
#define pcb_conflist_append rnd_conflist_append
#define pcb_conflist_nth rnd_conflist_nth
#define pcb_conflist_insert rnd_conflist_insert
#define pcb_conflist_remove rnd_conflist_remove
#define conf_hid_callbacks_s rnd_conf_hid_callbacks_s
#define conf_hid_callbacks_t rnd_conf_hid_callbacks_t
#define conf_hid_id_t rnd_conf_hid_id_t
#define pcb_conf_hid_set_data rnd_conf_hid_set_data
#define pcb_conf_hid_get_data rnd_conf_hid_get_data
#define pcb_conf_hid_set_cb rnd_conf_hid_set_cb
#define pcb_conf_hid_reg rnd_conf_hid_reg
#define pcb_conf_hid_unreg rnd_conf_hid_unreg
#define pcb_conf_pcb_hid_uninit rnd_conf_pcb_hid_uninit
#define conf_hid_local_cb rnd_conf_hid_local_cb
#define conf_hid_global_cb rnd_conf_hid_global_cb
#define conf_hid_global_cb_ptr rnd_conf_hid_global_cb_ptr
#define pcb_conf_loglevel_props rnd_conf_loglevel_props
#define pcb_conf_hid_global_cb_int rnd_conf_hid_global_cb_int
#define pcb_conf_hid_global_cb_ptr rnd_conf_hid_global_cb_ptr
#define pcb_trace rnd_trace
#define pcb_log_find_min rnd_log_find_min
#define pcb_log_find_min_ rnd_log_find_min_
#define pcb_log_find_first_unseen rnd_log_find_first_unseen
#define pcb_log_del_range rnd_log_del_range
#define pcb_log_export rnd_log_export
#define pcb_log_uninit rnd_log_uninit
#define PCB_MSG_DEBUG RND_MSG_DEBUG
#define PCB_MSG_INFO RND_MSG_INFO
#define PCB_MSG_WARNING RND_MSG_WARNING
#define PCB_MSG_ERROR RND_MSG_ERROR
#define pcb_message_level_t rnd_message_level_t
#define pcb_FS_error_message rnd_FS_error_message
#define pcb_logline_s rnd_logline_s
#define pcb_logline_t rnd_logline_t
#define pcb_open_error_message rnd_open_error_message
#define pcb_popen_error_message rnd_popen_error_message
#define pcb_opendir_error_message rnd_opendir_error_message
#define pcb_chdir_error_message rnd_chdir_error_message
#define pcb_log_next_ID rnd_log_next_ID
#define pcb_log_first rnd_log_first
#define pcb_log_last rnd_log_last
#define pcb_event_arg_s rnd_event_arg_s
#define pcb_event_arg_t rnd_event_arg_t
#define pcb_event_id_t rnd_event_id_t
#define EVENT_MAX_ARG RND_EVENT_MAX_ARG
#define pcb_event_argtype_t rnd_event_argtype_t
#define pcb_event_handler_t rnd_event_handler_t
#define pcb_event_bind rnd_event_bind
#define pcb_event_unbind rnd_event_unbind
#define pcb_event_unbind_cookie rnd_event_unbind_cookie
#define pcb_event_unbind_allcookie rnd_event_unbind_allcookie
#define pcb_event rnd_event
#define pcb_event_name rnd_event_name
#define pcb_event_app_reg rnd_event_app_reg
#define PCB_EVENT_GUI_INIT                RND_EVENT_GUI_INIT
#define PCB_EVENT_CLI_ENTER               RND_EVENT_CLI_ENTER
#define PCB_EVENT_TOOL_REG                RND_EVENT_TOOL_REG
#define PCB_EVENT_TOOL_SELECT_PRE         RND_EVENT_TOOL_SELECT_PRE
#define PCB_EVENT_TOOL_RELEASE            RND_EVENT_TOOL_RELEASE
#define PCB_EVENT_TOOL_PRESS              RND_EVENT_TOOL_PRESS
#define PCB_EVENT_BUSY                    RND_EVENT_BUSY
#define PCB_EVENT_LOG_APPEND              RND_EVENT_LOG_APPEND
#define PCB_EVENT_LOG_CLEAR               RND_EVENT_LOG_CLEAR
#define PCB_EVENT_GUI_LEAD_USER           RND_EVENT_GUI_LEAD_USER
#define PCB_EVENT_GUI_DRAW_OVERLAY_XOR    RND_EVENT_GUI_DRAW_OVERLAY_XOR
#define PCB_EVENT_USER_INPUT_POST         RND_EVENT_USER_INPUT_POST
#define PCB_EVENT_USER_INPUT_KEY          RND_EVENT_USER_INPUT_KEY
#define PCB_EVENT_CROSSHAIR_MOVE          RND_EVENT_CROSSHAIR_MOVE
#define PCB_EVENT_DAD_NEW_DIALOG          RND_EVENT_DAD_NEW_DIALOG
#define PCB_EVENT_DAD_NEW_GEO             RND_EVENT_DAD_NEW_GEO
#define PCB_EVENT_EXPORT_SESSION_BEGIN    RND_EVENT_EXPORT_SESSION_BEGIN
#define PCB_EVENT_EXPORT_SESSION_END      RND_EVENT_EXPORT_SESSION_END
#define PCB_EVENT_STROKE_START            RND_EVENT_STROKE_START
#define PCB_EVENT_STROKE_RECORD           RND_EVENT_STROKE_RECORD
#define PCB_EVENT_STROKE_FINISH           RND_EVENT_STROKE_FINISH
#define PCB_EVENT_SAVE_PRE                RND_EVENT_SAVE_PRE
#define PCB_EVENT_SAVE_POST               RND_EVENT_SAVE_POST
#define PCB_EVENT_LOAD_PRE                RND_EVENT_LOAD_PRE
#define PCB_EVENT_LOAD_POST               RND_EVENT_LOAD_POST
#define PCB_EVENT_BOARD_CHANGED           RND_EVENT_BOARD_CHANGED
#define PCB_EVENT_BOARD_META_CHANGED      RND_EVENT_BOARD_META_CHANGED
#define PCB_EVENT_BOARD_FN_CHANGED        RND_EVENT_BOARD_FN_CHANGED
#define PCB_FLT_FILE RND_FLT_FILE
#define pcb_file_loaded_s rnd_file_loaded_s
#define pcb_file_loaded_t rnd_file_loaded_t
#define pcb_file_loaded rnd_file_loaded
#define pcb_file_loaded_category rnd_file_loaded_category
#define pcb_file_loaded_clear rnd_file_loaded_clear
#define pcb_file_loaded_clear_at rnd_file_loaded_clear_at
#define pcb_file_loaded_set rnd_file_loaded_set
#define pcb_file_loaded_set_at rnd_file_loaded_set_at
#define pcb_file_loaded_del rnd_file_loaded_del
#define pcb_file_loaded_del_at rnd_file_loaded_del_at
#define pcb_file_loaded_init rnd_file_loaded_init
#define pcb_file_loaded_uninit rnd_file_loaded_uninit
#define pcb_file_loaded_type_t rnd_file_loaded_type_t
#define pcb_file_loaded_type_e rnd_file_loaded_type_e
#define PCB_FLT_CATEGORY RND_FLT_CATEGORY
#define pcb_fptr_t rnd_fptr_t
#define pcb_cast_f2d rnd_cast_f2d
#define pcb_cast_d2f rnd_cast_d2f
#define pcb_box_s rnd_rnd_box_s
#define pcb_box_t rnd_rnd_box_t
#define pcb_hidlib_s rnd_hidlib_s
#define pcb_angle_t rnd_angle_t
#define pcb_unit_s rnd_unit_s
#define pcb_unit_t rnd_unit_t
#define pcb_point_s rnd_point_s
#define pcb_point_t rnd_point_t
#define rnd_box_s rnd_rnd_box_s
#define rnd_box_t rnd_rnd_box_t
#define rnd_box_list_s rnd_rnd_box_list_s
#define rnd_box_list_t rnd_rnd_box_list_t
#define pcb_polyarea_s rnd_polyarea_s
#define pcb_polyarea_t rnd_polyarea_t
#define pcb_rtree_s rnd_rtree_s
#define pcb_rtree_t rnd_rtree_t
#define pcb_rtree_it_s rnd_rtree_it_s
#define pcb_rtree_it_t rnd_rtree_it_t
#define pcb_hid_cfg_s rnd_hid_cfg_s
#define pcb_hid_cfg_t rnd_hid_cfg_t
#define pcb_rtree_cardinal_t rnd_rtree_cardinal_t
#define pcb_rtree_coord_t rnd_rtree_coord_t
#define pcb_rtree_next rnd_rtree_next
#define pcb_rtree_first rnd_rtree_first
#define pcb_rtree_box_t rnd_rtree_box_t
#define pcb_rtree_all_next rnd_rtree_all_next
#define pcb_rtree_all_first rnd_rtree_all_first
#define pcb_rtree_dump_text rnd_rtree_dump_text
#define pcb_rtree_is_box_empty rnd_rtree_is_box_empty
#define pcb_rtree_search_any rnd_rtree_search_any
#define pcb_rtree_init rnd_rtree_init
#define pcb_rtree_uninit rnd_rtree_uninit
#define pcb_rtree_insert rnd_rtree_insert
#define pcb_rtree_delete rnd_rtree_delete
#define pcb_rtree_dir_t rnd_rtree_dir_t
#define PCB_RTREE_H RND_RTREE_H
#define pcb_RTREE_DIR_STOP rnd_RTREE_DIR_STOP
#define pcb_RTREE_DIR_FOUND rnd_RTREE_DIR_FOUND
#define pcb_rtree_search_obj rnd_rtree_search_obj
#define pcb_pixmap_s rnd_pixmap_s
#define pcb_pixmap_t rnd_pixmap_t
#define pcb_hid_dad_subdialog_s rnd_hid_dad_subdialog_s
#define pcb_hid_dad_subdialog_t rnd_hid_dad_subdialog_t
#define pcb_hid_expose_ctx_s rnd_hid_expose_ctx_s
#define pcb_hid_expose_ctx_t rnd_hid_expose_ctx_t
#define pcb_hid_s rnd_hid_s
#define pcb_hid_t rnd_hid_t
#define pcb_xform_s rnd_xform_s
#define pcb_xform_t rnd_xform_t
#define pcb_hid_gc_t rnd_hid_gc_t
#define hid_gc_s rnd_hid_gc_s
#define pcb_hid_attr_val_s rnd_hid_attr_val_s
#define pcb_hid_attr_val_t rnd_hid_attr_val_t
#define pcb_hid_attribute_s rnd_hid_attribute_s
#define pcb_hid_attribute_t rnd_hid_attribute_t
#define pcb_export_opt_s rnd_export_opt_s
#define pcb_export_opt_t rnd_export_opt_t
#define pcb_layer_id_t rnd_layer_id_t
#define pcb_layergrp_id_t rnd_layergrp_id_t
#define PCB_LARGE_VALUE RND_LARGE_VALUE
#define PCB_MAX_COORD RND_MAX_COORD
#define PCB_MIN_SIZE RND_MIN_SIZE
#define PCB_PATH_MAX RND_PATH_MAX
#define PCB_DYNFLAG_BLEN RND_DYNFLAG_BLEN
#define PCB_MAX_LINE_POINT_DISTANCE RND_MAX_LINE_POINT_DISTANCE
#define PCB_MAX_POLYGON_POINT_DISTANCE RND_MAX_POLYGON_POINT_DISTANCE
#define PCB_MAX_NETLIST_LINE_LENGTH RND_MAX_NETLIST_LINE_LENGTH
#define PCB_MIN_GRID_DISTANCE RND_MIN_GRID_DISTANCE
#define pcb_grid_t rnd_grid_t
#define pcb_grid_fit rnd_grid_fit
#define pcb_grid_parse rnd_grid_parse
#define pcb_grid_free rnd_grid_free
#define pcb_grid_append_print rnd_grid_append_print
#define pcb_grid_print rnd_grid_print
#define pcb_grid_set rnd_grid_set
#define pcb_grid_list_jump rnd_grid_list_jump
#define pcb_grid_list_step rnd_grid_list_step
#define pcb_grid_inval rnd_grid_inval
#define pcb_grid_install_menu rnd_grid_install_menu
#define pcb_cost_t rnd_heap_cost_t
#define pcb_heap_s rnd_heap_s
#define pcb_heap_t rnd_heap_t
#define pcb_heap_create rnd_heap_create
#define pcb_heap_destroy rnd_heap_destroy
#define pcb_heap_free rnd_heap_free
#define pcb_heap_insert rnd_heap_insert
#define pcb_heap_remove_smallest rnd_heap_remove_smallest
#define pcb_heap_replace rnd_heap_replace
#define pcb_heap_is_empty rnd_heap_is_empty
#define pcb_heap_size rnd_heap_size
#define pcb_hat_property_e rnd_hat_property_e
#define pcb_hat_property_t rnd_hat_property_t
#define pcb_hid_mouse_ev_t rnd_hid_mouse_ev_t
#define PCB_HATP_max RND_HATP_max
#define PCB_HATP_GLOBAL_CALLBACK RND_HATP_GLOBAL_CALLBACK
#define PCB_HID_MOUSE_PRESS RND_HID_MOUSE_PRESS
#define PCB_HID_MOUSE_RELEASE RND_HID_MOUSE_RELEASE
#define PCB_HID_MOUSE_MOTION RND_HID_MOUSE_MOTION
#define PCB_HID_MOUSE_POPUP RND_HID_MOUSE_POPUP
#define rnd_cap_invalid rnd_cap_invalid
#define rnd_cap_square rnd_cap_square
#define rnd_cap_round rnd_cap_round
#define rnd_cap_style_t rnd_cap_style_t
#define pcb_core_gc_t rnd_core_gc_t
#define pcb_hidval_t rnd_hidval_t
#define PCB_HIDCONCAT RND_HIDCONCAT
#define PCB_WATCH_READABLE RND_WATCH_READABLE
#define PCB_WATCH_WRITABLE RND_WATCH_WRITABLE
#define PCB_WATCH_ERROR RND_WATCH_ERROR
#define PCB_WATCH_HANGUP RND_WATCH_HANGUP
#define pcb_watch_flags_t rnd_watch_flags_t
#define PCB_HID_COMP_RESET RND_HID_COMP_RESET
#define PCB_HID_COMP_POSITIVE RND_HID_COMP_POSITIVE
#define PCB_HID_COMP_POSITIVE_XOR RND_HID_COMP_POSITIVE_XOR
#define PCB_HID_COMP_NEGATIVE RND_HID_COMP_NEGATIVE
#define PCB_HID_COMP_FLUSH RND_HID_COMP_FLUSH
#define pcb_composite_op_t rnd_composite_op_t
#define PCB_HID_BURST_START RND_HID_BURST_START
#define PCB_HID_BURST_END RND_HID_BURST_END
#define PCB_HID_ATTR_EV_WINCLOSE RND_HID_ATTR_EV_WINCLOSE
#define PCB_HID_ATTR_EV_CODECLOSE RND_HID_ATTR_EV_CODECLOSE
#define PCB_HID_FSD_READ RND_HID_FSD_READ
#define PCB_HID_FSD_MAY_NOT_EXIST RND_HID_FSD_MAY_NOT_EXIST
#define PCB_HID_FSD_IS_TEMPLATE RND_HID_FSD_IS_TEMPLATE
#define pcb_burst_op_t rnd_burst_op_t
#define pcb_burst_op_s rnd_burst_op_s
#define pcb_composite_op_s rnd_composite_op_s
#define pcb_hid_attr_ev_t rnd_hid_attr_ev_t
#define pcb_hid_fsd_flags_e rnd_hid_fsd_flags_e
#define pcb_hid_fsd_flags_t rnd_hid_fsd_flags_t
#define pcb_hid_fsd_filter_t rnd_hid_fsd_filter_t
#define pcb_hid_fsd_filter_any rnd_hid_fsd_filter_any
#define pcb_menu_prop_s rnd_menu_prop_s
#define pcb_menu_prop_t rnd_menu_prop_t
#define pcb_hid_clipfmt_e rnd_hid_clipfmt_e
#define pcb_hid_clipfmt_t rnd_hid_clipfmt_t
#define pcb_hid_dock_t rnd_hid_dock_t
#define PCB_HID_CLIPFMT_TEXT RND_HID_CLIPFMT_TEXT
#define PCB_HID_DOCK_TOP_LEFT RND_HID_DOCK_TOP_LEFT
#define PCB_HID_DOCK_TOP_RIGHT RND_HID_DOCK_TOP_RIGHT
#define PCB_HID_DOCK_TOP_INFOBAR RND_HID_DOCK_TOP_INFOBAR
#define PCB_HID_DOCK_LEFT RND_HID_DOCK_LEFT
#define PCB_HID_DOCK_BOTTOM RND_HID_DOCK_BOTTOM
#define PCB_HID_DOCK_FLOAT RND_HID_DOCK_FLOAT
#define PCB_HID_DOCK_max RND_HID_DOCK_max
#define pcb_dock_is_vert rnd_dock_is_vert
#define pcb_dock_has_frame rnd_dock_has_frame
#define pcb_hid_expose_cb_t rnd_hid_expose_cb_t
#define pcb_hid_expose_t rnd_hid_expose_t
#define pcb_gui rnd_gui
#define pcb_render rnd_render
#define pcb_exporter rnd_exporter
#define pcb_current_action rnd_current_action
#define pcb_pixel_slop rnd_pixel_slop
#define pcb_hid_prompt_for rnd_hid_prompt_for
#define pcb_hid_message_box rnd_hid_message_box
#define pcb_hid_progress rnd_hid_progress
#define PCB_HAVE_GUI_ATTR_DLG RND_HAVE_GUI_ATTR_DLG
#define pcb_nogui_attr_dlg_new rnd_nogui_attr_dlg_new
#define pcb_hid_dock_enter rnd_hid_dock_enter
#define pcb_hid_dock_leave rnd_hid_dock_leave
#define pcb_hid_redraw rnd_hid_redraw
#define pcb_hid_busy rnd_hid_busy
#define pcb_hid_notify_crosshair_change rnd_hid_notify_crosshair_change
#define pcb_hid_attr_ev_e rnd_hid_attr_ev_e
#define pcb_hatt_compflags_e rnd_hatt_compflags_e
#define pcb_hatt_compflags_t rnd_hatt_compflags_t
#define PCB_HATF_FRAME RND_HATF_FRAME
#define PCB_HATF_TIGHT RND_HATF_TIGHT
#define PCB_HATF_SCROLL RND_HATF_SCROLL
#define PCB_HATF_HIDE_TABLAB RND_HATF_HIDE_TABLAB
#define PCB_HATF_CLR_STATIC RND_HATF_CLR_STATIC
#define PCB_HATF_LEFT_TAB RND_HATF_LEFT_TAB
#define PCB_HATF_TREE_COL RND_HATF_TREE_COL
#define PCB_HATF_EXPFILL RND_HATF_EXPFILL
#define PCB_HATF_HIDE RND_HATF_HIDE
#define PCB_HATF_TOGGLE RND_HATF_TOGGLE
#define PCB_HATF_TEXT_TRUNCATED RND_HATF_TEXT_TRUNCATED
#define PCB_HATF_TEXT_VERTICAL RND_HATF_TEXT_VERTICAL
#define PCB_HATF_PRV_BOARD RND_HATF_PRV_BOARD
#define PCB_HATF_WIDTH_CHR RND_HATF_WIDTH_CHR
#define PCB_HATF_HEIGHT_CHR RND_HATF_HEIGHT_CHR
#define PCB_HATT_LABEL RND_HATT_LABEL
#define PCB_HATT_INTEGER RND_HATT_INTEGER
#define PCB_HATT_REAL RND_HATT_REAL
#define PCB_HATT_STRING RND_HATT_STRING
#define PCB_HATT_BOOL RND_HATT_BOOL
#define PCB_HATT_ENUM RND_HATT_ENUM
#define PCB_HATT_UNIT RND_HATT_UNIT
#define PCB_HATT_COORD RND_HATT_COORD
#define PCB_HATT_BUTTON RND_HATT_BUTTON
#define PCB_HATT_TREE RND_HATT_TREE
#define PCB_HATT_PROGRESS RND_HATT_PROGRESS
#define PCB_HATT_PREVIEW RND_HATT_PREVIEW
#define PCB_HATT_PICTURE RND_HATT_PICTURE
#define PCB_HATT_PICBUTTON RND_HATT_PICBUTTON
#define PCB_HATT_COLOR RND_HATT_COLOR
#define PCB_HATT_TEXT RND_HATT_TEXT
#define PCB_HATT_BEGIN_HBOX RND_HATT_BEGIN_HBOX
#define PCB_HATT_BEGIN_VBOX RND_HATT_BEGIN_VBOX
#define PCB_HATT_BEGIN_HPANE RND_HATT_BEGIN_HPANE
#define PCB_HATT_BEGIN_VPANE RND_HATT_BEGIN_VPANE
#define PCB_HATT_BEGIN_TABLE RND_HATT_BEGIN_TABLE
#define PCB_HATT_BEGIN_TABBED RND_HATT_BEGIN_TABBED
#define PCB_HATT_BEGIN_COMPOUND RND_HATT_BEGIN_COMPOUND
#define PCB_HATT_END RND_HATT_END
#define pcb_hid_attr_type_t rnd_hid_attr_type_t
#define pcb_hid_attr_type_e rnd_hid_attr_type_e
#define pcb_hatt_flags rnd_hatt_flags
#define pcb_hatt_table_cols rnd_hatt_table_cols
#define PCB_HATT_IS_COMPOSITE RND_HATT_IS_COMPOSITE
#define PCB_HAT_IS_STR RND_HAT_IS_STR
#define pcb_export_register_opts rnd_export_register_opts
#define pcb_export_remove_opts_by_cookie rnd_export_remove_opts_by_cookie
#define pcb_export_uninit rnd_export_uninit
#define pcb_hid_attr_node_s rnd_hid_attr_node_s
#define pcb_hid_attr_node_t rnd_hid_attr_node_t
#define hid_attr_nodes rnd_hid_attr_nodes
#define pcb_hid_usage rnd_hid_usage
#define pcb_hid_usage_option rnd_hid_usage_option
#define pcb_hid_attrdlg_num_children rnd_hid_attrdlg_num_children
#define pcb_attribute_dialog_ rnd_attribute_dialog_
#define pcb_attribute_dialog rnd_attribute_dialog
#define pcb_hid_compflag_bit2name rnd_hid_compflag_bit2name
#define pcb_hid_compflag_name2bit rnd_hid_compflag_name2bit
#define pcb_hid_export_opt_func_action_e rnd_hid_export_opt_func_action_e
#define pcb_hid_export_opt_func_action_t rnd_hid_export_opt_func_action_t
#define PCB_HIDEOF_USAGE RND_HIDEOF_USAGE
#define PCB_HIDEOF_DAD RND_HIDEOF_DAD
#define pcb_hid_export_opt_func_t rnd_hid_export_opt_func_t
#define pcb_hid_cfg_load rnd_hid_cfg_load
#define pcb_hid_cfg_load_lht rnd_hid_cfg_load_lht
#define pcb_hid_cfg_load_str rnd_hid_cfg_load_str
#define pcb_hid_cfg_text_value rnd_hid_cfg_text_value
#define pcb_hid_cfg_get_menu rnd_hid_cfg_get_menu
#define pcb_hid_cfg_get_menu_at rnd_hid_cfg_get_menu_at
#define pcb_hid_cfg_create_hash_node rnd_hid_cfg_create_hash_node
#define pcb_hid_cfg_extend_hash_node rnd_hid_cfg_extend_hash_node
#define pcb_hid_cfg_extend_hash_nodev rnd_hid_cfg_extend_hash_nodev
#define pcb_hid_cfg_dfs rnd_hid_cfg_dfs
#define pcb_hid_cfg_error rnd_hid_cfg_error
#define pcb_hid_cfg_map_anchor_menus rnd_hid_cfg_map_anchor_menus
#define pcb_hid_cfg_del_anchor_menus rnd_hid_cfg_del_anchor_menus
#define pcb_hid_cfg_action rnd_hid_cfg_action
#define hid_cfg_mouse_init rnd_hid_cfg_mouse_init
#define hid_cfg_mouse_action rnd_hid_cfg_mouse_action
#define pcb_hid_cfg_keys_init rnd_hid_cfg_keys_init
#define pcb_hid_cfg_keys_uninit rnd_hid_cfg_keys_uninit
#define pcb_hid_cfg_keys_add_under rnd_hid_cfg_keys_add_under
#define pcb_hid_cfg_keys_add_by_desc rnd_hid_cfg_keys_add_by_desc
#define pcb_hid_cfg_keys_add_by_desc_ rnd_hid_cfg_keys_add_by_desc_
#define pcb_hid_cfg_keys_add_by_strdesc rnd_hid_cfg_keys_add_by_strdesc
#define pcb_hid_cfg_keys_add_by_strdesc_ rnd_hid_cfg_keys_add_by_strdesc_
#define pcb_hid_cfg_keys_del_by_desc rnd_hid_cfg_keys_del_by_desc
#define pcb_hid_cfg_keys_gen_accel rnd_hid_cfg_keys_gen_accel
#define pcb_hid_cfg_keys_input rnd_hid_cfg_keys_input
#define pcb_hid_cfg_keys_input_ rnd_hid_cfg_keys_input_
#define pcb_hid_cfg_keys_action rnd_hid_cfg_keys_action
#define pcb_hid_cfg_keys_action_ rnd_hid_cfg_keys_action_
#define pcb_hid_cfg_keys_seq rnd_hid_cfg_keys_seq
#define pcb_hid_cfg_keys_seq_ rnd_hid_cfg_keys_seq_
#define PCB_M_Shift RND_M_Shift
#define PCB_M_Ctrl RND_M_Ctrl
#define PCB_M_Alt RND_M_Alt
#define PCB_M_Mod1 RND_M_Mod1
#define PCB_M_Release RND_M_Release
#define PCB_MB_LEFT RND_MB_LEFT
#define PCB_MB_MIDDLE RND_MB_MIDDLE
#define PCB_MB_RIGHT RND_MB_RIGHT
#define PCB_MB_SCROLL_UP RND_MB_SCROLL_UP
#define PCB_MB_SCROLL_DOWN RND_MB_SCROLL_DOWN
#define PCB_MB_SCROLL_LEFT RND_MB_SCROLL_LEFT
#define PCB_MB_SCROLL_RIGHT RND_MB_SCROLL_RIGHT
#define PCB_MB_ANY RND_MB_ANY
#define PCB_M_ANY RND_M_ANY
#define pcb_hid_cfg_mouse_t rnd_hid_cfg_mouse_t
#define hid_cfg_keyhash_t rnd_hid_cfg_keyhash_t
#define HIDCFG_MAX_KEYSEQ_LEN RND_HIDCFG_MAX_KEYSEQ_LEN
#define pcb_hid_cfg_keyseq_s rnd_hid_cfg_keyseq_s
#define pcb_hid_cfg_keyseq_t rnd_hid_cfg_keyseq_t
#define pcb_hid_cfg_keytrans_s rnd_hid_cfg_keytrans_s
#define pcb_hid_cfg_keytrans_t rnd_hid_cfg_keytrans_t
#define pcb_hid_cfg_keys_s rnd_hid_cfg_keys_s
#define pcb_hid_cfg_keys_t rnd_hid_cfg_keys_t
#define hid_cfg_key_default_trans rnd_hid_cfg_key_default_trans
#define pcb_hid_cfg_mod_t rnd_hid_cfg_mod_t
#define pcb_hid_text_set_t rnd_hid_text_set_t
#define PCB_HID_TEXT_INSERT RND_HID_TEXT_INSERT
#define PCB_HID_TEXT_REPLACE RND_HID_TEXT_REPLACE
#define PCB_HID_TEXT_APPEND RND_HID_TEXT_APPEND
#define PCB_HID_TEXT_MARKUP RND_HID_TEXT_MARKUP
#define pcb_hid_text_t rnd_hid_text_t
#define pcb_hid_row_t rnd_hid_row_t
#define pcb_hid_tree_t rnd_hid_tree_t
#define pcb_hid_preview_s rnd_hid_preview_s
#define pcb_hid_preview_t rnd_hid_preview_t
#define pcb_hid_compound_t rnd_hid_compound_t
#define PCB_DAD_DECL RND_DAD_DECL
#define PCB_DAD_DECL_NOINIT RND_DAD_DECL_NOINIT
#define PCB_DAD_FREE RND_DAD_FREE
#define PCB_DAD_NEW RND_DAD_NEW
#define PCB_DAD_DEFSIZE RND_DAD_DEFSIZE
#define PCB_DAD_MINSIZE RND_DAD_MINSIZE
#define PCB_DAD_RUN RND_DAD_RUN
#define PCB_DAD_AUTORUN RND_DAD_AUTORUN
#define PCB_DAD_CURRENT RND_DAD_CURRENT
#define PCB_DAD_BEGIN RND_DAD_BEGIN
#define PCB_DAD_BEGIN_TABLE RND_DAD_BEGIN_TABLE
#define PCB_DAD_BEGIN_TABBED RND_DAD_BEGIN_TABBED
#define PCB_DAD_BEGIN_HBOX RND_DAD_BEGIN_HBOX
#define PCB_DAD_BEGIN_VBOX RND_DAD_BEGIN_VBOX
#define PCB_DAD_END RND_DAD_END
#define PCB_DAD_COMPFLAG RND_DAD_COMPFLAG
#define PCB_DAD_LABEL RND_DAD_LABEL
#define PCB_DAD_LABELF RND_DAD_LABELF
#define PCB_DAD_ENUM RND_DAD_ENUM
#define PCB_DAD_BOOL RND_DAD_BOOL
#define PCB_DAD_INTEGER RND_DAD_INTEGER
#define PCB_DAD_REAL RND_DAD_REAL
#define PCB_DAD_COORD RND_DAD_COORD
#define PCB_DAD_STRING RND_DAD_STRING
#define PCB_DAD_TEXT RND_DAD_TEXT
#define PCB_DAD_BUTTON RND_DAD_BUTTON
#define PCB_DAD_BUTTON_CLOSE RND_DAD_BUTTON_CLOSE
#define PCB_DAD_BUTTON_CLOSES_NAKED RND_DAD_BUTTON_CLOSES_NAKED
#define PCB_DAD_BUTTON_CLOSES RND_DAD_BUTTON_CLOSES
#define PCB_DAD_PROGRESS RND_DAD_PROGRESS
#define PCB_DAD_PREVIEW RND_DAD_PREVIEW
#define PCB_DAD_PICTURE RND_DAD_PICTURE
#define PCB_DAD_PICBUTTON RND_DAD_PICBUTTON
#define PCB_DAD_COLOR RND_DAD_COLOR
#define PCB_DAD_BEGIN_HPANE RND_DAD_BEGIN_HPANE
#define PCB_DAD_BEGIN_VPANE RND_DAD_BEGIN_VPANE
#define PCB_DAD_TREE RND_DAD_TREE
#define PCB_DAD_TREE_APPEND RND_DAD_TREE_APPEND
#define PCB_DAD_TREE_APPEND_UNDER RND_DAD_TREE_APPEND_UNDER
#define PCB_DAD_TREE_INSERT RND_DAD_TREE_INSERT
#define PCB_DAD_TREE_SET_CB RND_DAD_TREE_SET_CB
#define PCB_DAD_DUP_ATTR RND_DAD_DUP_ATTR
#define PCB_DAD_DUP_EXPOPT RND_DAD_DUP_EXPOPT
#define PCB_DAD_MINVAL RND_DAD_MINVAL
#define PCB_DAD_MAXVAL RND_DAD_MAXVAL
#define PCB_DAD_MINMAX RND_DAD_MINMAX
#define PCB_DAD_CHANGE_CB RND_DAD_CHANGE_CB
#define PCB_DAD_RIGHT_CB RND_DAD_RIGHT_CB
#define PCB_DAD_ENTER_CB RND_DAD_ENTER_CB
#define PCB_DAD_HELP RND_DAD_HELP
#define PCB_DAD_DEFAULT_PTR RND_DAD_DEFAULT_PTR
#define PCB_DAD_DEFAULT_NUM RND_DAD_DEFAULT_NUM
#define PCB_DAD_SET_VALUE RND_DAD_SET_VALUE
#define PCB_DAD_UPDATE_INTERNAL RND_DAD_UPDATE_INTERNAL
#define PCB_DAD_ALLOC RND_DAD_ALLOC
#define PCB_DAD_SET_ATTR_FIELD RND_DAD_SET_ATTR_FIELD
#define PCB_DAD_OR_ATTR_FIELD RND_DAD_OR_ATTR_FIELD
#define PCB_DAD_SET_ATTR_FIELD_VAL RND_DAD_SET_ATTR_FIELD_VAL
#define PCB_DAD_SET_ATTR_FIELD_NUM RND_DAD_SET_ATTR_FIELD_NUM
#define PCB_DAD_SET_ATTR_FIELD_PTR RND_DAD_SET_ATTR_FIELD_PTR
#define PCB_DAD_FREE_FIELD RND_DAD_FREE_FIELD
#define PCB_DAD_WIDTH_CHR RND_DAD_WIDTH_CHR
#define pcb_dad_retovr_t rnd_dad_retovr_t
#define pcb_hid_dad_run rnd_hid_dad_run
#define pcb_hid_dad_close_cb rnd_hid_dad_close_cb
#define pcb_hid_dad_buttons_t rnd_hid_dad_buttons_t
#define pcb_dad_preview_zoomto rnd_dad_preview_zoomto
#define pcb_dad_tree_append rnd_dad_tree_append
#define pcb_dad_tree_append_under rnd_dad_tree_append_under
#define pcb_dad_tree_insert rnd_dad_tree_insert
#define pcb_dad_tree_free rnd_dad_tree_free
#define pcb_hid_dad_close rnd_hid_dad_close
#define pcb_hid_iterate rnd_hid_iterate
#define pcb_hid_export_opt_func_dad_s rnd_hid_export_opt_func_dad_s
#define pcb_hid_export_opt_func_dad_t rnd_hid_export_opt_func_dad_t
#define PCB_DAD_SPIN_INT RND_DAD_SPIN_INT
#define PCB_DAD_SPIN_DOUBLE RND_DAD_SPIN_DOUBLE
#define PCB_DAD_SPIN_COORD RND_DAD_SPIN_COORD
#define PCB_DAD_SPIN_FREQ RND_DAD_SPIN_FREQ
#define pcb_hid_dad_spin_t rnd_hid_dad_spin_t
#define PCB_DAD_SPIN_GET_TYPE RND_DAD_SPIN_GET_TYPE
#define PCB_DAD_SPIN_ANY RND_DAD_SPIN_ANY
#define rnd_hid_dad_spin_up rnd_hid_dad_spin_up
#define rnd_hid_dad_spin_down rnd_hid_dad_spin_down
#define rnd_hid_dad_spin_unit rnd_hid_dad_spin_unit
#define rnd_hid_dad_spin_unit rnd_hid_dad_spin_unit
#define pcb_hid_dad_spin_warn rnd_hid_dad_spin_warn
#define pcb_dad_spin_up_cb rnd_dad_spin_up_cb
#define pcb_dad_spin_down_cb rnd_dad_spin_down_cb
#define pcb_dad_spin_txt_enter_cb rnd_dad_spin_txt_enter_cb
#define pcb_dad_spin_txt_enter_cb_dry rnd_dad_spin_txt_enter_cb_dry
#define pcb_dad_spin_txt_enter_call_users rnd_dad_spin_txt_enter_call_users
#define pcb_dad_spin_txt_change_cb rnd_dad_spin_txt_change_cb
#define pcb_dad_spin_unit_cb rnd_dad_spin_unit_cb
#define pcb_dad_spin_free rnd_dad_spin_free
#define pcb_dad_spin_set_num rnd_dad_spin_set_num
#define pcb_dad_spin_widget_state rnd_dad_spin_widget_state
#define pcb_dad_spin_widget_hide rnd_dad_spin_widget_hide
#define pcb_dad_spin_set_value rnd_dad_spin_set_value
#define pcb_dad_spin_set_help rnd_dad_spin_set_help
#define pcb_dad_spin_set_geo rnd_dad_spin_set_geo
#define pcb_dad_spin_update_internal rnd_dad_spin_update_internal
#define pcb_dad_coord_spins rnd_dad_coord_spins
#define pcb_dad_tree_hide_all rnd_dad_tree_hide_all
#define pcb_dad_tree_unhide_filter rnd_dad_tree_unhide_filter
#define pcb_dad_tree_mkdirp rnd_dad_tree_mkdirp
#define pcb_dad_tree_new_row rnd_dad_tree_new_row
#define pcb_dad_tree_free_row rnd_dad_tree_free_row
#define pcb_dad_tree_parent_row rnd_dad_tree_parent_row
#define pcb_dad_tree_prev_row rnd_dad_tree_prev_row
#define pcb_dad_tree_next_row rnd_dad_tree_next_row
#define pcb_dad_tree_build_path rnd_dad_tree_build_path
#define pcb_dad_tree_set_hash rnd_dad_tree_set_hash
#define pcb_dad_tree_remove rnd_dad_tree_remove
#define pcb_dad_tree_clear rnd_dad_tree_clear
#define pcb_dad_tree_get_selected rnd_dad_tree_get_selected
#define pcb_dad_tree_update_hide rnd_dad_tree_update_hide
#define pcb_dad_tree_modify_cell rnd_dad_tree_modify_cell
#define pcb_dad_tree_jumpto rnd_dad_tree_jumpto
#define pcb_dad_tree_expcoll_ rnd_dad_tree_expcoll_
#define pcb_dad_tree_expcoll rnd_dad_tree_expcoll
