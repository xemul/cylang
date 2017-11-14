:- Args 2

= f <~ Args.1

= pop8 {
	= o ( )
	~ 8 {
		= c << f "c"
		:- c
		+) o c 
	}
	:= o
}

= chr { := ? & <= c 126 >= c 33 { := %% "\(c:%c)" } { := "." } }

= l 0
~ {
	= s1 :: pop8 [ f f ] :- -. s1
	= s2 :: pop8 [ f f ]

	= v "" = s ""
	~ s1 {	= v + v %% "\(_:%02x) "
		= s + s :: chr [ c _ ]
	}
	~ s2 {	= v + v %% " \(_:%02x)"
		= s + s :: chr [ c _ ]
	}

	` %% "\(l:%08x)  \(v)  |\(s)|"
	=^ l + _ 16
}
