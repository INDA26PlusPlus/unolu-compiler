# EINAPL
*EINAPL Is Not A Programming Language*

## Syntax
```
<top-level-decl> ::= ( ( <variable-decl> ";" ) | <function-decl> | ( <globalization-decl> ";" ) ) \[<top-level-decl>\]
<function-decl> ::= "Fun" <identifier> "(" <parameter>* ")" \[<type>\] "{" <block> "}"
<parameter> ::= <type> <variable>
<variable-decl> ::= <type> <variable>
<globalization-decl> ::= "Globalize" <identifier> <global-identifier>
<block> ::= <statement>*
<statement> ::= ( <function-call> | <yap> | <lockin> | <if-else> | <loop> | <throw> | <variable-decl> | <assignment> | "Break" ) ";"
<assignment> ::= "Assign" <expression> "To" <variable>
<if-else> ::= "If" <expression> "{" <block> "}" \[ "Else" "{" <block> "}" \]
<loop> ::= "Loop" "{" <block> "}"
<yap> ::= "Yap"\["67"\] (<variable> | <char-literal> | <integer-literal>)
<throw> ::= "Throw" (<variable> | <char-literal> | <integer-literal>)
<expression> ::= <logical>
<logical> ::= <comparison> ("And" | "Or") <logical>
<comparison> ::= <term> ("=" | "<" | ">") <comparison>
<term> ::= <bitwise> ("+" | "_" | "Mod") <term>
<bitwise> ::= <factor> ("&" | "|" | "^") <bitwise>
<factor> ::= <unary> ("\*" | "/" | "//") <factor>
<unary> ::= ["!" | "\~"] <primary>
<primary> ::= "(" <expression> ")" | <function-call> | <variable> | <integer-literal> | <char-literal>   
<function-call> ::= <identifier> "(" <argument>* ")"
<argument> ::= <variable> | <char-literal> | <integer-literal>
<lockin> ::= "LockIn"\["67"\] <variable>
<integer-literal> ::= \["-"\] ( "0" | \["1"-"9"\] <digit>* ) \["u"\]
<type> ::= "Ubyte" | "Sbyte" | "Uquad" | "Squad"
<variable> ::= "$" <identifier>
<identifier> ::= <letter> (<letter> | <digit> | "-")*
<global-identifier> ::= "." <letter>+
<letter> ::= \["a"-"z"\] | \["A"-"Z"\]
<digit> ::= \["0"-"9"\]
<char-literal> ::= "'" <ascii-character> "'"
```

## Semantics
* In expressions, all operations are 32 bit, they can still be saved to 8 bit variables through.
* The `mod` operator is always unsigned.
* Yap and LockIn print/read a single char.
* Yap67 and LockIn67 print/read an integer.

## Common errors when writing EINAPL code
* If statements and loops require a semicolon after the closing brace.
* Equality checks use a single equal sign.
* There are no escape sequences, if you want a new line then you actually have to write a new line character.


## Additional Language Notes
* Signed division uses "/" and unsigned division uses "//"
* Comments are single line and start with "@"


## Areas of Improvement
* Make all the stack implementations into one.
* Implement a assembly peephole optimizer.

## Example usage
To calculate a fibonacci number you can compile `fib.einapl` with the command `einaplc fib.einapl -o fib`.
Executing `./fib` will prompt you to enter a number n, the program with then print the nth fibonacci number.

