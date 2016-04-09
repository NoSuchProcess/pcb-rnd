(PkgLoad "pcb-rnd-gpmi/actions" 0)
(PkgLoad "pcb-rnd-gpmi/dialogs" 0)

(define ev_action (lambda (id name argc x y)
  (dialog_report "Greeting window" "Hello world!")))

(define ev_gui_init (lambda (id argc argv)
  (create_menu "/main_menu/Plugins/GPMI scripting/hello" "hello()" "h" "Ctrl<Key>w" "tooltip for hello")))

(Bind "ACTE_action" "ev_action")
(Bind "ACTE_gui_init" "ev_gui_init")
(action_register "hello" "" "log hello world" "hello()")
