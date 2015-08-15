(PkgLoad "pcb-rnd-gpmi/actions" 0)
(PkgLoad "pcb-rnd-gpmi/dialogs" 0)

(define ev_action (lambda (id name argc x y)
  (dialog_report "Greeting window" "Hello world!")))

(Bind "ACTE_action" "ev_action")
(action_register "hello" "" "log hello world" "hello()")

