= elf_hdr (
	( "ident_l" "l" )
	( "ident_h" "l" )
	( "type"    "s" )
	( "mach"    "s" )
	( "vers"    "i" )
)

= read_file {
	= ret [ ]
	~ struct {
		= f _.0
		= ret..f << file _.1
	}
	:= ret
}

= type [
	"0" "None"
	"1" "Rel"
	"2" "Exec"
	"3" "Dyn"
	"4" "Core"
]

= mach [
	"62" "x86_64"
]

= field_str {
	= key %% "\(key)"
	:= .| map..key key
}

= eh : read_file [ struct elf_hdr file <~ Args.1 ]

` + "Type: " : field_str [ key eh.type map type ]
` + "Mach: " : field_str [ key eh.mach map mach ]
` + "Vers: " %% "\(eh.vers)"
