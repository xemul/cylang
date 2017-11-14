= expr Args.1 :- expr
= tokens %/ expr

= nt {
	= t -( tokens
	:= ? {
		== t "+" {: + : nt __ : nt __ }
		== t "-" {: - : nt __ : nt __ }
		== t "*" {: * : nt __ : nt __ }
		== t "/" {: / : nt __ : nt __ }
		== t "%" {: % : nt __ : nt __ }
		_+ {: %~ t }
	}
}

= x : nt __
` %% "\(x)"
