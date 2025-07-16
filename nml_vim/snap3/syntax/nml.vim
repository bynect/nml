" Vim syntax file
" Language:		NML
" Maintainer:	Federico Angelilli <code@fdrang.com>
"
" vim: sw=2 ts=4

" Syntax file alredy loaded
if exists("b:current_syntax")
  finish
endif

" Save cpo
let s:keepcpo = &cpo
set cpo&vim

" Case sensitive
syntax case match
setlocal iskeyword+='

" Delimiters
" FIXME: Improve delimiter handling
syntax match nmlErrorParen ")"
syntax region nmlDelim transparent matchgroup=nmlKeywordDelim start="(" matchgroup=nmlKeywordDelim end=")" contains=ALLBUT,@nmlErrorParen

syntax match nmlErrorBrack "\]"
syntax region nmlDelim transparent matchgroup=nmlKeywordDelim start="\[" matchgroup=nmlKeywordDelim end="\]" contains=ALLBUT,@nmlErrorBrack

" Comments
syntax keyword nmlNote contained TODO FIXME XXX NOTE
syntax region nmlComment start="(\*" end="\*)" contains=@Spell,nmlComment,nmlNote

" Keywords
syntax keyword nmlKeyword fun let in match with when as
syntax keyword nmlKeyword and if then else type data

" TODO: Other keywords
" syntax keyword nmlKeyword end forall exists sig

" TODO: Effect keywords
" syntax keyword nmlKeyword effect do handle

" TODO: Module system keywords
" syntax keyword nmlKeyword module import export

" Special characters
syntax match nmlKeychar "|"
syntax match nmlKeychar ":"
syntax match nmlKeychar "\\"
syntax match nmlKeychar ";"

" Operators
syntax match nmlArrow "->"
syntax match nmlArrow "<-"
syntax match nmlCons "::"

syntax match nmlOperator "="
syntax match nmlOperator "*"
syntax match nmlOperator ","
syntax match nmlOperator "`\w*`"

" TODO: Add all operators

" Constructors
syntax match nmlConstructor "\u\(\w\|'\)*\>"
syntax match nmlNil "\[\s*\]"

" Types
syntax keyword nmlType Unit Int Float String Char Bool
syntax keyword nmlType List Option Either

" Literals
syntax match nmlUnit "(\s*)"
syntax match nmlInt "-\=\<\d\(_\|\d\)*[l|L|n]\?\>"
syntax match nmlInt "-\=\<0[x|X]\(\x\|_\)\+[l|L|n]\?\>"
syntax match nmlFloat "-\=\<\d\(_\|\d\)*\.\?\(_\|\d\)*\([eE][-+]\=\d\(_\|\d\)*\)\=\>"
syntax region nmlString start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=@Spell
syntax match nmlChar "'\\\d\d\d'\|'\\[\'ntbr]'\|'.'"
syntax keyword nmlBool True False

" Highlighting
highlight def link nmlErrorParen Error
highlight def link nmlErrorParen Error
highlight def link nmlKeywordDelim nmlDelim
highlight def link nmlDelim Keyword

highlight def link nmlNote Todo
highlight def link nmlComment Comment

highlight def link nmlKeyword Keyword
highlight def link nmlKeychar Keyword

highlight def link nmlArrow Keyword
highlight def link nmlCons Operator
highlight def link nmlOperator Operator

highlight def link nmlConstructor Constant
highlight def link nmlNil nmlConstructor

highlight def link nmlType Type

highlight def link nmlUnit Constant
highlight def link nmlInt Number
highlight def link nmlFloat Float
highlight def link nmlString String
highlight def link nmlChar Character
highlight def link nmlBool Boolean

let b:current_syntax = 'nml'

" Reset cpo
let &cpo = s:keepcpo
unlet s:keepcpo
