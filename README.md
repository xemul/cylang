## CY -- a just for fun programming language

The project goal was to have some fun by implementing an interpreted
programming language without a single keyWORD. All language-specific
instructions are symbols from the nowadays keyboard.

To simplify the syntax scanner, the program is considered to be a
sequence of "tokens" separated with space, tab of a newline, when
met while scanning the program, each token is "evaluated". Some
tokens are evaluated immediately into some value, some need one or
more further tokens (evaluated too). In simple words -- it's a
prefix notation syntax.

If a token starts with a digit it's considered to be an integer.

If a token starts with a letter (any case) it's considered to be
a symbol. Symbols may include letters, digits, underbars and dots
inside. No other characters are allowed.

If a token is `(` it's a list, if a token is `[` it's a map and if
a token is `{` it's a commands block. They start the respective set
(or collection) and terminate at the next paired closing brace.

Otherwise a token is considered to be a command.

Two exceptions are made in the space-separation rule above -- if
a token starts with a `"` or `'` then it's considered to be a string
constant and the token ends with the next `"` or `'` respectively. All
spaces, tabs and even newlines are included in the string as is.
No backslashes are supported (yet), they are included in the string
as is. The 2nd exception is token started with `#`. It's a comment
and it terninates at the next `#` (not at the end of line).

Some tokens may evaluate a boolean value or a special `NOVALUE` thing.
The latter is the default evaluation result for any token unless 
explicitly documented.

Tokens may also result in a stream value, which represents an open
file, pipe, socket, etc.

To sum up -- a token can be an integer, a string, a symbol, define
list, map or a command block, be a stream, and NOVALUE or to be a
command (or a comment, but these are ignored).

### Numbers and strings

These are evaluated immediately into the respective value. Quotes for
strings are not included into it.

### Lists

List is evaluated by evaluating all the subsequent tokens untill the `)`
one, which denotes the end of the list. "Subsequent evaluation" literally
means evaluation, e.g. the `( + 1 2 )` means a list of one element being
the sum of 1 and 2.

### Maps

Maps always use strings as keys and are evaluated by evaluating all the
subsequent tokens in pairs, considering the first one to be the string and
the second one to be any other token. The first one is thus a key and the
second one is the vlaue.

For simplicity in describing and using objects the key may not be a string
token (i.e. it may not start with `"` or `'`), but can be a symbol without
dots. In either case the key token is treated as just a string. IOW two next
tokens mean the same

    [ "a" 1 ]
    [ a 1 ]

Mind the space before the final `]`, as the latter must be a separate token.

### Command blocks

Like lists, command block token grabs all the subsequent tokens until the `}`
one. Note, that it only grabs the tokens as is, without evaluating them. To
evaluate the tokens in a command block you should pass control to it with
one of the commands described below.

### Symbols

Symbols are like variable names. They are always evaluated into the value they
refer to (except in a single case when a new symbol is declared). Symbols
can be considered to be pointers to other objects.

Characters `_` and `.` are special in a symbol name. If a symbol starts with a
`_` it's one the service symbols. These are

* `_` -- a cursor. This is a symbol that refers to the current element in the
loop, map/filter and swap commands (see below)

* `_-` and `_+` -- false and true constants respectively

* `_?` -- a random number (unlimited, use the mod operation to trim it)

* `_:` -- current namespace (see below)

* `_<`, `_>` and `_>!` -- stdin, stdout and stderr streams respectively

An underbar anywhere inside symbol name is considered to be just the part of
the name.

A dot (`.`) inside symbol is used to split a symbol name into pieces each
of which is considered to be a key in the collection (list or map) that has
been referenced by the previous part of the symbol. For example the symbol
`a.b` means find the object named "a", then find an object named "b" in it,
provided `a` referred to a map.

Note, that the value "b" is treated as the key value. If you want to treat
"b" as another symbol that cotains the key value, you should use two dots as
separator. For example `a..b` means -- find the object "a", then find the
symbol "b", get what it refers to (it should be a string), then find in "a"
the value by the obtained string key.

Dot can follow a cursor, for example the symbol `_.a` means take the cursor
object, treat it as a map and find object "a" in it.

For lists the "key" is expected to be a number, for example `a.0` means 
find list named "a" and get the 0th element from it. Respectively `a..b`
means find list "a", then get what b refers to (should be a number) then
get the b-th element.

To declare a new symbol there's a `!` command, find its description below.

## Command tokens

As was said above, commands may need more subsequent tokens to be evaluated.
Here's how.

### Declaring a symbol

Token `!` makes a new symbol. It takes a symbol token, and dereferences it all
excluding the traling component. The result should be a map. Then it evaluates 
the next token and creates a new element in the map referenced by the 1st token
with the key being the trailing component of it and the value being the 2nd token.

The question is -- if all the symbols are created in some map, where would a
symbol without dots be created. The ansewer is -- there's an implicit root map,
so if the symbol name doesn't have any dots in it the new entry is created in
this root map thus resulting in somehwat that looks like a global variable. 
For example the `! x 1` makes a variable named "x" being a number 1. Respectively

    ! x [ ]
    ! x.a "string"

makes a new empty map named "x", then makes an entry with the key "a" in it being
a string with the value "string".

Since all symbols start from the root map, it's possible to change this map into some
other map. This is done with the `:` command. It evaluates the next map token and
sets it as the root one. All the service variables mentioned above remain accessible
in any map, the `_:` can be used to return back to the original namespace.

### Arythmetics

Tokens: `+ - / * %`. Evaluate two more tokens. If the next tokens are numbers they 
are added, substracted, etc. The result is the number as well. Otherwise some magic
comes up.

 \+ on two strings concatenates those and results in a new string
 
 \+ on two lists splices them and results in a new list

 \- on a list and a number removes the n-th element from the list
 
 \- on a map and a string removes the respective key from the map

### Boolean

Tokens: `& | ^ ~`. All but `~` need two more boolean tokens, `~` needs one.
Do what they are expected to and result in a boolean value. Note, that for
`&` and `|` both argument tokens ARE evaluated before doing the operation,
it's not like && and || in C.

### Lists

Tokens: `( ) (: (< (> (<> +( +) -( -)`. Tokens `(` and `)` mark the list
start and end respectively. Other tokens typically need at least one more 
list token.

`(:` generates a new list. It continuously evaluates the next token until it
results in a NOVALUE, and results in a list of generated values.

`(>`, `(<` and `(<>` cut the list and result in a new one. Evaluate one list token
and one (or two for the `<>` one) number(s). The resulting list is cut from head, 
tail or both respectively. Right index is included, left index is not. Rule of a
thumb is `>` results in the right (tail) part of the list and `<` results in the
left (head) part of the list.

`+(` and `+)` are push to head and push to tail respectively. Evaluate list token 
and one more one of any type, then add the latter value to the former list. The
result is NOVALUE, the 1st list token is modified in place.

Similarly `-(` and `-)` are pops from head or tail. Evaluate one list token and 
modify it in place. Both result in whatever is poped from the list or in a NOVALUE 
if the list was empty.

### Operations with strings

Tokens: `%% %~ %/ %^`. All evaluate one more token string to work on.

`%%` results in a new formatted string. Formatting means scanning the argument
string and getting the `\(...)` pieces from it. Each `...` piece is then
considered to be a symbol which is dereferenced and instead of the whole `\(...)` 
block the string representation of it is inserted. E.g.

    %% "Hello, \(name)!"
    
will dereference the "name" symbol and, if it's a string "world", will result in
a new string "Hello, world!"

`%~` is the atio (or strtol) command. Results in a number value corresponding to
the argument string.

`%/` splits the argument string using spaces and tabs as separator. The result is
a list of strings.

`%^` and `%$` evaluates one more string and evaluate a boolean value meaning whether
the first argument respectively starts of ends with the second one.

### Checks, if-s and loops

Check tokens: `= != > >= < <=`. Evaluate two more tokens of the same type, compare 
them and retuls in a boolean value.

Ifs and loops tokens: `? ?? ~`. All evaluate one or more command blocks and
may pass control to them. All typically result in NOVALUE, but if one of the
return commands is met in the command block, the if/loop token is evaluated into
its argument (see more details further).

The `?` evaluates three tokens -- a boolean one and two command blocks. If the bool
is true the control is passed to the first block, otherwise to the second.

The `??` token is the multiple choice token. It evaluates one command block token
and considers it to consist of bool:block pairs. The first boolean token evaluated
into true value passes control to the respective block token, then the whole ??
evaluation stops.

Loop is `~`. It evaluates the next token and grabs one more. For a list it calls
the 2nd block for each list element. For a map it calls the block for each map
value, for command block it calls one untill it results in NOVALUE and calls
2nd block.

### Printing

Tokens: <code>`</code> and <code>``</code>. Evaluate next token and print it on the
screen. The former one accepts only strings and prints them as is, the latter one
accepts any other token and prints it some string representation, e.g. strings are
printed with quotes aound and new lines at the end.

### Command blocks

Tokens: `{ } -> . <! <+ <-`. The `{` and `}` denote start and end of the command
block, token `.` is a shortcut for the `{ }` pair and means a no-op block (useful
as `?` and `??` sub-blocks). Tokens `->`, `<!`, `<+` and `<-` switch between 
blocks.

The `->` one evaluates a command block token and a map token, sets the map as the 
namespace, passes control to the command block, then restores the namespace back. 
It's thus the way to do a function call with arguments. Using `_:` namespace makes 
smth like goto.

The `<!` is like return or break in C. It evaluates the next token, then completes
execution of the current command block and (!) makes the evaluated argument be the 
result of evaluation of the token that caused the execution of this command block.

E.g. the `-> { <- 1 }` makes the first `->` be evaluated into 1. The `? _+ { <- 1 } .`
makes `?` be evaluated into 1, since it's `?` that caused execution of the block
with `<-`.

The `<+` token is used to propagate the "return" one more code block up. It evaluates
the next token, if it's a NOVALUE then the `<+` does nothing, otherwise it acts
as `<!` stopping execution of current block and evaluating it's caller into the value.

The `<-` works the other way -- it stops execution of the current command block
is the argument is NOVALUE, otherwise does nothing. The result of this token is
always NOVALUE.

### Streams

Tokens: `<~ <~> ~> <-> ->> <->>` to open a stream and `<< >>` to read from and
write to a stream.

Opening tokens evaluate the string token and open the file by the string name.
Mode is r, r+, w, w+, a and a+ from fopen man page respectively. The result is
stream value.

Reading from stream evaluates a stream token and a string token that shows what
to read. Currently the following formats are supported:

* "ln" results in string with a single line read from file (excluding the trailing
newline character).

* "b", "s", "i" and "l" read 8, 16, 32 and 64-bit numbers respectively.

### Miscelaneous

Tokens: `@ $ ;= ;- |`.

The `@` evaluates a list token and one more token and checks whether the latter
one is present in the list. Results in a boolean value.

The `$` evaluates the next token, checks it to be list, map or string and results
in a number value equal to the size of the argument.

The `;=` evaluates two next tokens. If the first one is NOVALUE, the it results
in the 2nd value, otherwise `;` results in the 1st value. It's token meaning is
the "default value".

The `;-` evaluates next token and results in NOVALUE if it's empty, i.e. a NOVALUE
itself or false bool, zero number, empty string, list or map. Otherwise results
in the mentioned token value. The token's meaning is "novalue if empty".

`|` converts (maps or filters) a list or a map. It evaluates two next tokens and
then for each element from the 1st (it should be a list or a map) calls the 2nd
one (it should be a command block). If the command block returns a value (with
it becomes a value of the result, if it just finishes -- the result is not modified.
For maps the key to be used is inherited from the original map.

Roughly speaking convertion token is equivalent to the list loop, with the
exception that it results in a new collection and return from the commands block
doesn't abort the conversion.

The `!%` token is re-declare one. It works like the `!` one, but unlike it, the
`!%` results in the old value of the symbol being declared. If th symbol didn't
exist before, the result is NOVALUE. When evaluating the velue for the new
symbol the cursor is available and points to the symbol's old value. E.g. the
`! x !% y + _ 1` is equivalent to C `x = y++`.

## Launching

The `cyvm` binary syntax is

* `cyvm -c` prints the commands and their short meaning
* `cyvm -r file.cy` runs the program in a file
* `cyvm -t file.cy` prints the found tokens without evaluating them

When running a program execution starts by evaluating the very first token. A special
name `Args` is available in the root map and is the list of CLI arguments (including
the .cy file name).

Each token has a location in the file in the `@line.number` format. The `-t` option
prints tokens and their location. When an error in evaluation occurs the cyvm prints
the fauling token location and exits.

## What else

Memory management is absent. There's no explicit malloc/free commands, objects are
allocated transparently and are not freed :) garbage collection, well, may come some
day.

Error handling is not there yet. In some situations (e.g. out-of-bounds list access or
getting a missing key from the map) the result is NOVALUE, in all other cyvm just exits.

Doing recursion looks clumsy, look at examples/qsort-rec.cy file's line

    -> split [ split split words b ]

it does the recursive call, but to make the split function call itself it pushes its
name into the namespace it will live in.

## Command names

Current set of commands is subject to change. The plan is to make some symbol reflect
the command type. E.g.

* `?` for conditional ops (if, select)
* `~` for loops of all kinds
* `(` and `)` for lists
* `[` and `]` for maps
* `{` and `}` for command blocks
* `%` for strings
* `!` for errors (not there yet) (now used for declare)
* `<` and `>` for streams
* math and bool are very common and shouldn't change
