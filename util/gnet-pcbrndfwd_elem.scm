;;; gEDA - GPL Electronic Design Automation
;;; gnetlist - gEDA Netlist
;;; Copyright (C) 1998-2008 Ales Hvezda
;;; Copyright (C) 1998-2008 gEDA Contributors (see ChangeLog for details)
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
;;; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

;; PCB forward annotation script;
;; modified version of pcbrndfwd: do not do the net list, only elements,
;; this way nets can be imported in the traditional netlist-way

(use-modules (ice-9 regex))
(use-modules (ice-9 format))

;; This is a list of attributes which are propogated to the pcb
;; elements.  Note that refdes, value, and footprint need not be
;; listed here.
(define pcbrndfwd:element-attrs
  '("device"
    "manufacturer"
    "manufacturer_part_number"
    "vendor"
    "vendor_part_number"
    ))

(define (pcbrndfwd:quote-string s)
  (string-append "\""
		 (regexp-substitute/global #f "\"" s 'pre "\\\"" 'post)
		 "\"")
  )


(define (pcbrndfwd:each-attr refdes attrs port)
  (if (not (null? attrs))
      (let ((attr (car attrs)))
	(format port "ElementSetAttr(~a,~a,~a)~%"
		(pcbrndfwd:quote-string refdes)
		(pcbrndfwd:quote-string attr)
		(pcbrndfwd:quote-string (gnetlist:get-package-attribute refdes attr)))
	(pcbrndfwd:each-attr refdes (cdr attrs) port))))

;; write out the pins for a particular component
(define pcbrndfwd:component_pins
  (lambda (port package pins)
    (if (and (not (null? package)) (not (null? pins)))
	(begin
	  (let (
		(pin (car pins))
		(label #f)
		(pinnum #f)
		)
	    (display "ChangePinName(" port)
	    (display (pcbrndfwd:quote-string package) port)
	    (display ", " port)

	    (set! pinnum (gnetlist:get-attribute-by-pinnumber package pin "pinnumber"))

	    (display pinnum port)
	    (display ", " port)

	    (set! label (gnetlist:get-attribute-by-pinnumber package pin "pinlabel"))
	    (if (string=? label "unknown") 
		(set! label pinnum)
		)
	    (display (pcbrndfwd:quote-string label) port)
	    (display ")\n" port)
	    )
	  (pcbrndfwd:component_pins port package (cdr pins))
	  )
	)
    )
  )

(define (pcbrndfwd:each-element elements port)
  (if (not (null? elements))
      (let* ((refdes (car elements))
	     (value (gnetlist:get-package-attribute refdes "value"))
	     (footprint (gnetlist:get-package-attribute refdes "footprint"))
	     )

	(format port "ElementList(Need,~a,~a,~a)~%"
		(pcbrndfwd:quote-string refdes)
		(pcbrndfwd:quote-string footprint)
		(pcbrndfwd:quote-string value))
	(pcbrndfwd:each-attr refdes pcbrndfwd:element-attrs port)
	(pcbrndfwd:component_pins port refdes (gnetlist:get-pins refdes))

	(pcbrndfwd:each-element (cdr elements) port))))

(define (pcbrndfwd_elem output-filename)
  (let ((port (open-output-file output-filename)))
    (format port "ElementList(Start)\n")
    (pcbrndfwd:each-element packages port)
    (format port "ElementList(Done)\n")
    (close-output-port port)))
