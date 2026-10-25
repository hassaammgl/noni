" P3.4 ^ gg G oracle — run with:
"   nvim --headless -u NONE -es -S tests/diff/gen_motion_p3d.vim
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

" --- ^ first non-blank ---
call Run('caret_first_nonblank', ['  hello'], '^', 1, 1)
call Run('caret_on_word_start', ['hello'], '^', 1, 1)
call Run('caret_mid_word', ['  hello'], '^', 1, 4)
call Run('caret_on_blank_mid', ['   a  b'], '^', 1, 6)
call Run('caret_count_down', ['  a', '   b', 'c'], '2^', 1, 3)
call Run('caret_on_empty', ['ab', ''], '^', 2, 1)
call Run('caret_blank_only_line', ['ab', '   '], '^', 2, 1)
call Run('caret_tabs', ["\tx y"], '^', 1, 1)
call Run('caret_count_no_down', ['ab', 'cd'], '4^', 1, 2)
call Run('caret_goes_down_then_back', ['  aa', ' x'], '2^', 1, 1)

" --- gg (g-prefix counts too) ---
call Run('gg_first_line_nonblank', ['  indent', 'x'], 'gg', 1, 1)
call Run('gg_from_mid_somewhere', ['  indent', 'x'], 'gg', 2, 1)
call Run('gg_count3', ['a', 'b', '  cc'], '3gg', 1, 1)
call Run('gg_count_clamps', ['a', 'b', '  cc'], '9gg', 1, 1)
call Run('gg_on_empty_first', ['', 'x'], 'gg', 1, 1)
call Run('gg_to_empty_line', ['a', '', '  cc'], '2gg', 1, 1)
call Run('gg_tabs_first', ["\tind", 'x'], 'gg', 1, 1)

" --- G last line / count ---
call Run('G_last_line_nonblank', ['a', 'b  c'], 'G', 1, 1)
call Run('G_count3', ['one', 'two', '  three', 'four'], '3G', 1, 1)
call Run('G_count_clamps', ['one', 'two', '  three', 'four'], '12G', 1, 1)
call Run('G_on_empty_last', ['a', ''], 'G', 1, 1)
call Run('G_count_empty_line', ['a', 'b', ''], '3G', 1, 1)
call Run('G_from_mid', ['one', 'two', 'three'], 'G', 1, 2)
call Run('G_to_blank_only', ['a', '   '], 'G', 1, 1)

" --- columns carried after gg/G (curswant probes) ---
call Run('ggj_col_carry', ['  aa', '  bbb', 'x'], 'ggj', 1, 1)
call Run('Gj_col_carry', ['ab', '  longer', '    third'], 'Gj', 1, 1)
call Run('Gk_col_carry', ['      top', '  longer', 'bottom'], 'Gk', 1, 3)

" --- ACCEPT-regression: $ on a tab line (P2.5 motion over display cols) ---
call Run('dollar_on_tab_line', ["\tabc", 'x'], '$', 1, 1)
call Run('dollar_count_on_tab_line', ["\tabc", 'x'], '2$', 1, 5)

call writefile(g:fixture, 'tests/diff/motion_p3d.fxt')
quit!
