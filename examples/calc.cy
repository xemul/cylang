= expr Args.1 :- expr
= tokens %/ expr

= nt {
	= t -( tokens
	:= ? {
		@ ( "+" "-" "/" "*" "%" ) t {: ` t : nt __ : nt __ }
		_+ {: %~ t }
	}
}

= x : nt __
; %% "\(x)"
