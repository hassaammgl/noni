" P3.9 phase-check sweep oracle: repeated (idempotent) motions, motions that
" hit the buffer edge, and counts far larger than the buffer. Run with:
"   nvim --headless -u NONE -es -S tests/diff/gen_motion_p3i.vim
"
" Same deterministic view setup and record format as gen_motion_p3f.vim.
let g:fixture = ['fixture: name keys startLine,startCol endLine,endCol top=<end topline> | init @ final']
function! Run(name, init, keys, l, c) abort
  execute 'set lines=24'
  enew!
  for line in a:init
    call append(line('$'), line)
  endfor
  0d _
  let rows = winheight(0)
  let top = max([1, a:l - (rows - 1)])
  let &scroll = rows / 2
  call winrestview({'lnum': a:l, 'col': a:c - 1, 'topline': top})
  call feedkeys(a:keys, "xt")
  let ls = join(getline(1, line('$')), '|')
  let kk = substitute(a:keys, "\e", '<Esc>', 'g')
  let kk = substitute(kk, "\x06", '<C-f>', 'g')
  let kk = substitute(kk, "\x02", '<C-b>', 'g')
  let kk = substitute(kk, "\x04", '<C-d>', 'g')
  let kk = substitute(kk, "\x15", '<C-u>', 'g')
  call add(g:fixture, printf('%s %s %d,%d %d,%d top=%d | %s @ %s',
      \ a:name, escape(kk, '|'), a:l, a:c, line('.'), col('.'),
      \ line('w0'), join(a:init, '|'), ls))
endfunction

" --- repeated motions (the second application must be a no-op where vim's
"     motion is idempotent, and a plain second step where it is not) --------
let L = ['abcdefghij', 'ab', 'abcdefghij']
call Run('twice_dollar', L, '$$', 1, 4)
call Run('twice_zero', L, '00', 1, 6)
call Run('twice_caret', ['  abcd', 'abcdefghij'], '^^', 1, 1)
call Run('twice_G', L, 'GG', 1, 2)
call Run('twice_gg', L, 'gggg', 3, 2)
call Run('twice_zz', L, 'zzzz', 2, 1)
call Run('twice_H', L, 'HH', 3, 1)
call Run('twice_L', L, 'LL', 1, 1)
call Run('twice_M', L, 'MM', 1, 1)
call Run('twice_j_last', L, 'jj', 3, 1)
call Run('twice_k_first', L, 'kk', 1, 1)
call Run('twice_zt', L, 'ztzt', 3, 1)
call Run('twice_bracket', ['if (a) {'], '%%', 1, 4)
call Run('twice_brace', ['a', '', 'b', '', 'c'], '}}', 1, 1)
call Run('twice_brace_close', ['a', '', 'b', '', 'c'], '{{', 1, 1)
call Run('twice_percent_line', map(range(1, 40), '"P" . v:val'), '50%50%', 1, 1)
call Run('twice_w_eof', ['ab'], 'www', 1, 2)
call Run('twice_b_bof', ['ab'], 'bbb', 1, 1)

" --- counts far beyond the buffer ----------------------------------------
let p5 = ['a', 'b', 'c', 'd', 'e']
call Run('count_j_overflow', p5, '999j', 1, 1)
call Run('count_k_overflow', p5, '999k', 5, 3)
call Run('count_G_overflow', p5, '999G', 1, 1)
call Run('count_gg_overflow', p5, '999gg', 5, 2)
call Run('count_dollar_overflow', p5, '999$', 1, 1)
call Run('count_w_overflow', ['aa bb cc'], '999w', 1, 1)
call Run('count_b_overflow', ['aa bb cc'], '999b', 1, 7)
call Run('count_e_overflow', ['aa bb cc'], '999e', 1, 1)
call Run('count_brace_overflow', ['a', '', 'b', '', 'c'], '999}', 1, 1)
call Run('count_braceback_overflow', ['a', '', 'b', '', 'c'], '999{', 1, 1)
call Run('count_pct_overflow', map(range(1, 40), '"P" . v:val'), '999%', 20, 2)
call Run('count_H_overflow', map(range(1, 30), '"H" . v:val'), '999H', 20, 1)
call Run('count_L_overflow', map(range(1, 30), '"L" . v:val'), '999L', 20, 1)
call Run('count_M_overflow', map(range(1, 30), '"M" . v:val'), '999M', 20, 1)
call Run('count_zt_overflow', map(range(1, 30), '"Z" . v:val'), '999zt', 20, 1)
call Run('count_zz_overflow', map(range(1, 30), '"Z" . v:val'), '999zz', 20, 1)
call Run('count_find_missing', ['abc'], '999fz', 1, 1)
call Run('count_find_present', ['aXbXc', 'abcd'], '999fX', 1, 1)
call Run('count_semicolon_none', ['abc'], '2;', 1, 1)

" --- edge shapes ----------------------------------------------------------
call Run('empty_buffer_j', [''], 'j', 1, 1)
call Run('empty_buffer_G', [''], 'G', 1, 1)
call Run('empty_buffer_dollar', [''], '$', 1, 1)
call Run('empty_buffer_match', [''], '%', 1, 1)
call Run('blank_lines_brace', ['', '', ''], '}', 1, 1)

call writefile(g:fixture, 'tests/diff/motion_p3i.fxt')
quit!
