= split {
	= s -( words :- s

	= a ( ) = b ( )
	~ { := -( words } {
		+) ? > _ s { := a } { := b } _
	}

	:: split [ split split words b ]
	` s
	:: split [ split split words a ]
}

= words ( )
~ { := << _< "ln" } { +) words _ }
:: split __
