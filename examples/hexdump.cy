<+ < $ Args 2

! f <~ Args.1
! c (: << f "c"
! pop8 {
	! n 0
	! o ( )
	~? < n 8 {
		! x -( i <- x
		+) o x
		! n + n 1
	}
	<! o
}

! l 0
~? _+ {
	<- c.0

	! v ""
	! s1 -> pop8 [ i c ]
	! s2 -> pop8 [ i c ]
	~( s1 { ! v + v %% "\(_:%02x) " }
	~( s2 { ! v + v %% " \(_:%02x)" }

	! s ""
	~( + s1 s2 { ! s + s ? & <= _ 126 >= _ 33 { <! %% "\(_:%c)" } { <! "." } }

	` + %% "\(l:%08x)  " + v + "  |" + s "|"

	! l + l 16
}