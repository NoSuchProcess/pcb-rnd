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

;; Simplified (thrown out m4 support) specially for Igor2 by vzh

;; Splits a string with space separated words and returns a list
;; of the words (still as strings).
(define (gsch2pcb:split-to-list the-string)
  (filter!
   (lambda (x) (not (string=? "" x)))
   (string-split the-string #\space)))

;; Write the footprint for the package `refdes' to `port'.
(define (gsch2pcb:write-value-footprint refdes port)

  (let ((value (gnetlist:get-package-attribute refdes "value"))
         (footprint (gsch2pcb:split-to-list
                     (gnetlist:get-package-attribute refdes "footprint"))))

    (format port "PKG(~A,~A,~A)\n" (string-join footprint " ") refdes value)))

;; Write the footprints for all the refdes' in `lst'.
(define (gsch2pcb:write-value-footprints port lst)
  (for-each (lambda (x) (gsch2pcb:write-value-footprint x port)) lst))


(define (gsch2pcb output-filename)
  (begin
;;    (set-current-output-port (gnetlist:output-port output-filename))
    (set-current-output-port (open-output-file output-filename))

    ;; don't use m4
    (gsch2pcb:write-value-footprints (current-output-port) packages)))
