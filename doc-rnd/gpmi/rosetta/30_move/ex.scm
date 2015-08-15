(PkgLoad "pcb-rnd-gpmi/actions" 0)
(PkgLoad "pcb-rnd-gpmi/dialogs" 0)
(PkgLoad "pcb-rnd-gpmi/layout" 0)

(define ev_action (lambda (id name argc x y)
(let
	(
		(dx (* (string->number (action_arg 0)) (string->number (mm2pcb_multiplier))))
		(dy (* (string->number (action_arg 1)) (string->number (mm2pcb_multiplier))))
		(num_objs (string->number (layout_search_selected "mv_search" "OM_ANY")))
		(obj_ptr 0)
	)
	(do ((n 0 (1+ n)))
	    ((> n num_objs))
		(set! obj_ptr (layout_search_get "mv_search" n))
		(layout_obj_move obj_ptr "OC_OBJ" (inexact->exact dx) (inexact->exact dy))
	)
)))

(Bind "ACTE_action" "ev_action")
(action_register "mv" "" "move selected objects by dx and dy mm" "mv(dx,dy)")
