; "want: STRING"
= x "STRING"
; x

; "want: 1 2 3"
~ ( 1 2 3 ) { ;; _ }

; "want: 1 3"
~ ( ( 1 2 ) ( 3 4 ) ) { ;; _.0 }

; "want: 2 4"
= n 1
~ ( ( 1 2 ) ( 3 4 ) ) { ;; _..n }

= x ( ( 1 2 ) ( 3 4 ) )
= n 1
; "LIST"
; "want: (1,2,) (3,4,)"
;; x.0
;; x..n
; "want: 1 3 2 4"
;; x.0.0
;; x..n.0
;; x.0..n
;; x..n..n

= x [ a 1 b 2 ]
= n "b"
; "MAP"
; "want: 1 2"
;; x.a
;; x..n

= x [ a ( 1 2 ) b ( 3 4 ) ]
= n "b"
= m 1
; "MAP of LIST"
; "want: 1 3 2 4"
;; x.a.0
;; x..n.0
;; x.a..m
;; x..n..m

= x ( [ a 1 b 2 ] [ a 3 b 4 ] )
= n 1
= m "b"
; "LIST of MAP"
; "want: 1 3 2 4"
;; x.0.a
;; x..n.a
;; x.0..m
;; x..n..m
