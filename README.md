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

If a token is ( it's a list, if a token is [ it's a map and if
a token is { it's a commands block.

Otherwise a token is considered to be a command.

Two exceptions are made in the space-separation rule above -- if
a token starts with a " or ' then it's considered to be a string
constant and the token ends with the next " or ' respectively. All
spaces, tabs and even newlines are included in the string as is.
No backslashes are supported (yet), they are included in the string
as is. The 2nd exception is token started with #. It's a comment
and it terninates at the next # (not at the end of line).

To sum up -- a token can be an integer, a string, a symbol, define
list, map or a command block or to be a command (or a comment, but
these are ignored). Also some tokens may evaluate a boolean value or
a special NOVALUE thing. Here's how they are evaluated.

### Numbers and strings

These are evaluated immediately and just result in the respective value.

### Lists

List is evaluated by evaluating all the subsequent tokens untill the )
one which denotes the end of the list.

### Maps

Maps always use strings as keys and are evaluated by evaluating all the
subsequent tokens in pairs, considering the first one to be the key and
the second one to be any other token.

For simplicity in describing and using objects the key may not be a string
token (startin with " or '), but be a symbol (without dots). In any case
the key token is treated as just a string. IOW two next tokens mean the same

    [ "a" 1 ]
    [ a 1 ]

Mind the space before the final ], as the latter must be a separate token.

### Command blocks

Like lists, command block token grabs all the subsequent tokens until the }
one. Note, that it only grabs the tokens as is, without evaluating them. To
evaluate the tokens in a command block you should pass control to it with
one of the commands below.

### Symbols

Symbols are variable names. They are always evaluated into the value they
refer to (except in a single case when a new symbol is declared). Symbols
can be considered to be pointers to other objects.

Characters _ and . are special in a symbol name. If a symbol starts with a
_ it's one the service symbols. These are

    * _ -- a cursor. This is a symbol that refers to the current
     element in the list loop, list map or list filter (see below)
     command

    * _- and _+ -- false and true constants respectively

    * _? -- a random number (unlimited)

    * _: -- current namespace (see below)

An underbar anywhere inside symbol name is considered to be just the
part of the name.

A dot (.) inside symbol is used to split a symbol name into pieces each
of which is considered to be a key in the previously referenced object,
which in turn is expected to be a map or a list. For example the symbol
`a.b` means find the object named "a", then find an object named "b" in it
(provided a referred to a map).

Note, that the value "b" is considered to be the key. If you want to
treat "b" as another symbol that cotains the key value you can use two
dots as separator. For example `a..b` means -- find the object "a", then 
find the symbol "b", get what it refers to (it should be a string), then 
find in "a" the value by the obtained string key.

Dot can follow a cursor, for example the symbol `_.a` means take the cursor
object, treat it as a map and find object "a" in it.

For lists the "key" is expected to be a number, for example `a.0` means 
find list named "a" and get the 0th element from it. Respectively `a..b`
means find list "a", then get what b refers to (should be a number) then
get the b-th element.

To declare a new symbol there's a ! command, find its description below.

### Commands

As was said above, commands may need more subsequent tokens to be evaluated.
Here's how.

* Arythmetics

Tokens: `+ - / * %`. Evaluate two more tokens.

If the next tokens are numbers they are added, substracted, etc. Otherwise
some magic comes up.

 \+ on two strings concatenates those and results in a new string
 
 \+ on two lists splices them and results in a new list

 \- on a list and a number removes the n-th element from the list
 
 \- on a map and a string removes the respective key from the map

* Boolean

Tokens: `& | ^ ~`. All but ~ need two more boolean tokens, ~ needs one.
Do what they are expected to, but note, that for & and | both tokens ARE
evaluated before doing the operation, it's not like && and || in C.

* Lists

Tokens: `( ) (: (| (- (< (> (<> +( +) -( -)`. Tokens ( and ) mark the list
start and end respectively. Other tokens typically need at least one more 
list token.

`(:` generates a new list. It needs 3 more number tokens and makes a new 
list with numbers starting from the 1st number ending below the 3rd one 
with the 2nd number being an incremental step (works only for positive 
values).

`(|` maps a list. It takes one list token and one other token, then for each
element from the 1st one calls the 2nd token (to refer to the list element 
to convert use a cursor) then puts into the resulting list.

`(-` filters a list. Works similarly to map, but the 2nd token should result
in a boolean true or false value meaning that the respective element (a cursor)
should be included into the new list or not.

`(>`, `(<` and `(<>` cut the list (and produce a new one). Take one list token
and one (or two for the `<>` one) number(s). New list is cut from head, tail 
or both respectively up to, but not including, the numbered position(s).

`+(` and `+)` are push to head and push to tail respectively. Take list token 
and any other one and add the latter to the former.

Similarly `-(` and `-)` are pops from head or tail. Take one list token and 
modify it. Evaluated into whatever is poped from the list or a special NOVALUE 
if the list is empty.

* Operations with strings

`%%` taken a string token and generates a new formatted one. Formatting means
scanning the string and getting `\(...)` pieces from it. Each `...` piece is
then considered to be a symbol which gets evaluated and instead of the whole
`\(...)` block the string representation of it is inserted. E.g.

    %% "Hello, \(name)!"
    
will evaluate the "name" symbol and, if it's a string "world", will result in
a new string "Hello, world!"

`%~` is the atio (or strtol) command. It takes a string and coverts it into
a number value.

`%/` takes the string and splits it using spaces and tabs as separator. The
evaluated value is a list with strings.

`%^` and `%$` take two strings and evaluate a boolean value meaning whether the
1st string respectively starts of ends with the second one.

... TO BE CONTINUED
