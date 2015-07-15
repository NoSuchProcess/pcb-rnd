;;; -*-scheme-*-
;;;

;;; gEDA - GPL Electronic Design Automation
;;; gnetlist - gEDA Netlist
;;; Copyright (C) 1998-2010 Ales Hvezda
;;;
;;; This program is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
;;; MA 02111-1301 USA.

;;  gsch2pcb format  (based on PCBboard format by JM Routoure & Stefan Petersen)
;;  Bill Wilson    billw@wt.net
;;  6/17/2003

(use-modules (ice-9 popen))
(use-modules (ice-9 syncase))

;;
;;
(define gsch2pcb:write-top-header
  (lambda (port)
    (display "# release: pcb 1.99x\n" port)
    (display "# To read pcb files, the pcb version (or the" port)
    (display " cvs source date) must be >= the file version\n" port)
    (display "FileVersion[20070407]\n" port)
    (display "PCB[\"\" 600000 500000]\n" port)
    (display "Grid[10000.000000 0 0 0]\n" port)
    (display "Cursor[0 0 0.000000]\n" port)
    (display "PolyArea[200000000.000000]\n" port)
    (display "Thermal[0.500000]\n" port)
    (display "DRC[1000 1000 1000 1000 1500 1000]\n" port)
    (display "Flags(\"nameonpcb,uniquename,clearnew,snappin\")\n" port)
    (display "Groups(\"1,c:2:3:4:5:6,s:7:8\")\n" port)
    (display "Styles[\"Signal,1000,3600,2000,1000:" port)
    (display "Power,2500,6000,3500,1000:" port)
    (display "Fat,4000,6000,3500,1000:" port)
    (display "Skinny,600,2402,1181,600\"]\n" port)
))

;;
;;
(define gsch2pcb:write-bottom-footer
  (lambda (port)
    (display "Layer(1 \"top\")\n(\n)\n" port)
    (display "Layer(2 \"ground\")\n(\n)\n" port)
    (display "Layer(3 \"signal2\")\n(\n)\n" port)
    (display "Layer(4 \"signal3\")\n(\n)\n" port)
    (display "Layer(5 \"power\")\n(\n)\n" port)
    (display "Layer(6 \"bottom\")\n(\n)\n" port)
    (display "Layer(7 \"outline\")\n(\n)\n" port)
    (display "Layer(8 \"spare\")\n(\n)\n" port)
    (display "Layer(9 \"silk\")\n(\n)\n" port)
    (display "Layer(10 \"silk\")\n(\n)" port)
    (newline port)))

;;
;;

;; Splits a string with space separated words and returns a list
;; of the words (still as strings).
(define (gsch2pcb:split-to-list the-string)
  (filter!
   (lambda (x) (not (string=? "" x)))
   (string-split the-string #\space)))

;; Check if `str' contains only characters valid in an M4 function
;; name.  Note that this *doesn't* check that str is a valid M4
;; function name.
(define gsch2pcb:m4-valid?
  (let ((rx (make-regexp "^[A-Za-z0-9_]*$")))
    (lambda (str)
      (regexp-exec rx str))))

;; Quote a string to protect from M4 macro expansion
(define (gsch2pcb:m4-quote str)
  (string-append "`" str "'"))

;; Write the footprint for the package `refdes' to `port'.  If M4
;; footprints are enabled, writes in a format suitable for
;; macro-expansion by M4.  Any footprint names that obviously can't be
;; M4 footprints are protected from macro-expansion.
(define (gsch2pcb:write-value-footprint refdes port)

  (let* ((value (gnetlist:get-package-attribute refdes "value"))
         (footprint (gsch2pcb:split-to-list
                     (gnetlist:get-package-attribute refdes "footprint")))
         (fp (car footprint))
         (fp-args (cdr footprint))

         (nq (lambda (x) x)) ; A non-quoting operator
         (q (if gsch2pcb:use-m4 gsch2pcb:m4-quote nq))) ; A quoting operator

    (format port "~A(~A,~A,~A~A)\n"
            ;; If the footprint is obviously not an M4 footprint,
            ;; protect it from macro-expansion.
            ((if (gsch2pcb:m4-valid? fp) nq q) (string-append "PKG_" fp))
            (q (string-join footprint "-"))
            (q refdes)
            (q value)
            (string-join (map q fp-args) "," 'prefix))))

;; Write the footprints for all the refdes' in `lst'.
(define (gsch2pcb:write-value-footprints port lst)
  (for-each (lambda (x) (gsch2pcb:write-value-footprint x port)) lst))

;;
;;

(define gsch2pcb:use-m4 #f)

;; Macro that defines and sets a variable only if it's not already defined.
(define-syntax define-undefined
  (syntax-rules ()
    ((_ name expr)
     (define name
       (if (defined? (quote name))
           name
           expr)))))

;; Let the user override the m4 command, the directory
;; where pcb stores its m4 files and the pcb config directory.
(define-undefined gsch2pcb:pcb-m4-command "/usr/bin/m4")
(define-undefined gsch2pcb:pcb-m4-dir "/usr/share/pcb/m4")
(define-undefined gsch2pcb:m4-files "")

;; Let the user override the m4 search path
(define-undefined gsch2pcb:pcb-m4-path '("$HOME/.pcb" "."))

;; Build up the m4 command line
(define (gsch2pcb:build-m4-command-line output-filename)
  (string-append gsch2pcb:pcb-m4-command
                 " -d"
                 (string-join (cons gsch2pcb:pcb-m4-dir
                                    gsch2pcb:pcb-m4-path)
                              " -I" 'prefix)
                 " " gsch2pcb:pcb-m4-dir "/common.m4"
                 " " gsch2pcb:m4-files
                 " - >> " output-filename))

(define (gsch2pcb output-filename)
  (define command-line (gsch2pcb:build-m4-command-line output-filename))

  (let ((port (open-output-file output-filename)))
    (gsch2pcb:write-top-header port)
    (close-port port)
    )

  (format #t
"=====================================================
gsch2pcb backend configuration:

   ----------------------------------------
   Variables which may be changed in gafrc:
   ----------------------------------------
   gsch2pcb:pcb-m4-command:    ~S
   gsch2pcb:pcb-m4-dir:        ~S
   gsch2pcb:pcb-m4-path:       ~S
   gsch2pcb:m4-files:          ~S

   ---------------------------------------------------
   Variables which may be changed in the project file:
   ---------------------------------------------------
   gsch2pcb:use-m4:            ~A

   ----------------
   M4 command line:
   ----------------
   ~A

=====================================================
"
   gsch2pcb:pcb-m4-command
   gsch2pcb:pcb-m4-dir
   gsch2pcb:pcb-m4-path
   gsch2pcb:m4-files
   (if gsch2pcb:use-m4 "yes" "no")
   command-line)

  ;; If we have defined gsch2pcb:use-m4 then run the footprints
  ;; through the pcb m4 setup.  Otherwise skip m4 entirely
  (if gsch2pcb:use-m4
      ;; pipe with the macro define in pcb program
      (let ((pipe (open-output-pipe command-line))
	    )
	
	(display "Using the m4 processor for pcb footprints\n")
	;; packages is a list with the different refdes value
	(gsch2pcb:write-value-footprints pipe packages)
	(close-pipe pipe)
	)
      
      (let ((port  (open output-filename (logior O_WRONLY O_APPEND))))
	(display "Skipping the m4 processor for pcb footprints\n")
	(gsch2pcb:write-value-footprints port packages)
	(close-port port)
	)
      )

  (let ((port (open output-filename (logior O_WRONLY O_APPEND))))
    (gsch2pcb:write-bottom-footer port)
    close-port port)
  )

