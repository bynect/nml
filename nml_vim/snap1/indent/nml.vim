" Vim indent file
" Language: Nml
" Filenames: *.nml
"
" vim: sw=2

" Indentation file alredy loaded
if exists("b:did_indent")
  finish
endif
let b:did_ident = 1

setlocal tabstop=2
setlocal shiftwidth=2
setlocal expandtab
setlocal nolisp
setlocal nosmartindent

" TODO: Auto indentation function
setlocal noautoindent
