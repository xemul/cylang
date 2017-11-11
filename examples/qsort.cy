! sep {
	! a ( )
	! b ( )
	! s words.0
	~( (> words 1 {
		? > _ s { +) a _ } { +) b _ }
	}

	! ret ( )
	? >= $ b 1 { +) ret b } .
	+) ret ( s )
	? >= $ a 1 { +) ret a } .

	<! ret
}

! qsort {
	! wa ( words )
	! more _+
	~+ ;- more {
		! wr ( )
		! more _-
		~( wa {
			! words _
			? > $ words 1 {
				! wr + wr -> sep _:
				! more _+
			} {
				+) wr words
			}
		}

		! wa wr
	}

	<! | wa { <! _.0 }
}

! words (> Args 1
~( -> qsort _: { ` _ }
