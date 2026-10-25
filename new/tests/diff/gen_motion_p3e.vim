" P3.5 { } paragraph oracle — run with:
"   nvim --headless -u NONE -es -S tests/diff/gen_motion_p3e.vim
let g:fixture = ['fixture: name keys startLine,startCol endLine,endCol | init @ final']
function! Run(name, init, keys, l, c) abort
  %d _
  for line in a:init
    call append(line('$'), line)
  endfor
  0d _
  call cursor(a:l, a:c)
  call feedkeys(a:keys, "xt")
  let ls = join(getline(1, line('$')), '|')
  let kk = substitute(a:keys, "\e", '<Esc>', 'g')
  call add(g:fixture, printf('%s %s %d,%d %d,%d | %s @ %s',
      \ a:name, escape(kk, '|'), a:l, a:c, line('.'), col('.'),
      \ join(a:init, '|'), ls))
endfunction

" --- } forward ---
call Run('brace_fwd_basic', ['aa bb', '   ', 'cc dd'], '}', 1, 1)
call Run('brace_fwd_col_carry', ['one  two', '', 'zzzzzzz'], '}', 1, 5)
call Run('brace_fwd_multi_blank', ['aa', '', '', 'bb'], '}', 1, 1)
call Run('brace_fwd_on_blank', ['aa', '', 'bb'], '}', 2, 1)
call Run('brace_fwd_on_blank_blanks', ['aa', '  ', 'bb'], '}', 2, 1)
call Run('brace_fwd_from_here', [' ab c', '', 'x y z'], '}', 1, 4)
call Run('brace_double', ['a', '', 'b', '', 'c'], '}}', 1, 1)
call Run('brace_count2', ['a', '', 'b', '', 'c'], '2}', 1, 1)
call Run('brace_fwd_onto_trailing_blanks', ['aa', '', 'bb', ''], '}}', 1, 1)
call Run('brace_fwd_to_eof_blanks', ['aa', '   '], '}', 1, 1)
call Run('brace_fwd_empty_buffer', [''], '}', 1, 1)
call Run('brace_fwd_no_move_at_eof', ['aa', '', 'bb'], '}}}', 3, 1)

" --- { backward ---
call Run('brace_back_basic', ['cc dd', '   ', 'aa bb'], '{', 3, 1)
call Run('brace_back_col_carry', ['kkkkkkk', '', 'two  three'], '{', 3, 6)
call Run('brace_back_multi_blank', ['aa', '   ', '   ', 'bb'], '{', 4, 1)
call Run('brace_back_on_blank', ['aa', '', 'c'], '{', 2, 1)
call Run('brace_back_from_here', [' ab c', '', 'x y z'], '{', 3, 4)
call Run('brace_double_back', ['a', '', 'b', '', 'c'], '{{', 5, 1)
call Run('brace_back_count2', ['a', '', 'b', '', 'c'], '2{', 5, 1)
call Run('brace_back_onto_leading_blanks', ['', 'bb'], '{', 2, 1)
call Run('brace_back_to_eof_top_blank', ['   ', 'aa'], '{', 2, 1)
call Run('brace_back_empty_buffer', [''], '{', 1, 1)
call Run('brace_back_no_move_at_bof', ['aa', '', 'bb'], '{{{', 1, 1)

call writefile(g:fixture, 'tests/diff/motion_p3e.fxt')
quit!
