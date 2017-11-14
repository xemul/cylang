= x ( 1 2 3 )

# "Map example" #
= map_l {
	= ret ( )
	~ l { +) ret : fn [ x _ ] }
	:= ret
}

`` : map_l [ l x fn { := + x x } ]

# "Filter example" #
= filter_l {
	= ret ( )
	~ l { ? : fn [ x _ ] { +) ret _ } . }
	:= ret
}

`` : filter_l [ l x fn { := > x 1 } ]

# "Filter + map example #
= filter_map_l {
	= ret ( )
	~ l {
		= x : fn [ x _ ]
		? =. x . { +) ret x }
	}
	:= ret
}

`` : filter_map_l [ l x fn { := ? > x 1 { := + x x } . } ]
