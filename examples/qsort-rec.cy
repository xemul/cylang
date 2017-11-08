! split {
	?? {
		= 0 $ words .
		= 1 $ words { ` words.0 }
		_+ {
			! s words.0 ! a ( ) ! b ( )
			~( (> words 1 {
				? > _ s { +) a _ } { +) b _ }
			}

			-> split [ split split words b ]
			` s
			-> split [ split split words a ]
		}
	}
}

! words (> Args 1 -> split _:
