= split {
	= s -( words :- s

	= a ( ) = b ( )
	~ {
		= w -) words :- w
		+) ? > w s { := a } { := b } w
	}

	: split [ split split words b ]
	` s
	: split [ split split words a ]
}

= words ( )
~ { = l << _< "ln" :- l +) words l }
: split __
