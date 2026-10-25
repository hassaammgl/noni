" P2.7 motion oracle — run with:
"   nvim --headless -u NONE -es -S tests/diff/gen_motion.vim
" Records (name, start line/col, keys) -> resulting line/col + final text.
let g:fixture = ['fixture: name keys startLine,startCol endLine,endCol | lines = "|"-joined']
function! Run(name, init, keys, l, c) abort
  %d _
  for line in a:init
    call append(line('$'), line)
  endfor
  0d _
  call cursor(a:l, a:c)
  call feedkeys(a:keys, "xt")
  let ls = join(getline(1, line('$')), '|')
  " Esc bytes are written as the readable token <Esc> (parsed by the test).
  " Format: | init-picture @ final-picture (both '|'-joined line vectors).
  let kk = substitute(a:keys, "\e", '<Esc>', 'g')
  call add(g:fixture, printf('%s %s %d,%d %d,%d | %s @ %s',
      \ a:name, escape(kk, '|'), a:l, a:c, line('.'), col('.'),
      \ join(a:init, '|'), ls))
endfunction

call Run('h_at_col0', ['twochars'], 'h', 1, 1)
call Run('h2_at_col1', ['twochars'], '2h', 1, 2)
call Run('l_to_last_char', ['twochars'], 'l', 1, 1)
call Run('l_at_last_char', ['twochars'], 'l', 1, 9)
call Run('dollar_beyond_eol_clamps', ['twochars'], '5$', 1, 1)
call Run('dollar_on_empty_line', ['a', '', 'b'], '$', 2, 1)
call Run('j_to_shorter_line_clamps_col', ['longish', 'ab'], 'j', 1, 5)
call Run('k_to_shorter_line_clamps_col', ['ab', 'longish'], 'k', 2, 5)
call Run('fivej_stops_at_last_line', ['a', 'b', 'c'], '5j', 1, 1)
call Run('threej_count_prefix', ['l1', 'l2', 'l3', 'l4'], '3j', 1, 1)
call Run('ninej_crosses_eof_clamps', ['a', 'b', 'c'], '9j', 1, 1)
call Run('zero_goes_col0', ['one two'], '0', 1, 5)
call Run('zero_after_count_is_col0', ['one two'], '20', 1, 5)
call Run('dollar_midline', ['one two'], '$', 1, 2)
call Run('h_on_empty_line', [''], 'h', 1, 1)
call Run('l_on_empty_line', [''], 'l', 1, 1)
call Run('j_on_single_line', ['solo'], 'j', 1, 1)
call Run('x_at_eol_noop', ['ab'], 'x', 1, 3)
call Run('count3x_deletes_three', ['abcdef'], '3x', 1, 2)
call Run('A_appends_at_eol', ['ab'], "AX\<Esc>", 1, 2)
call Run('A_on_empty_line', [''], "AX\<Esc>", 1, 1)
call Run('i_types_at_cursor', ['ab'], "iX\<Esc>", 1, 2)
call Run('j_col_clamp_persists_via_k_back', ['longish', 'ab'], 'jk', 1, 5)
call Run('j_streak_then_k_recovers_want', ['longish', 'ab'], 'jjk', 1, 5)
call Run('h_resets_desired_col', ['longish', 'ab'], 'jhj', 1, 5)
call Run('count2_dollar', ['ab', 'cd'], '2$', 1, 1)
call Run('dollar_overflow_clamps_last_line', ['ab', 'cd', 'ef'], '4$', 1, 1)
call Run('dollar_overflow_at_last_line_rejects', ['ab', 'cd'], '5$', 2, 1)
call Run('dollar_count1_on_empty_line', ['a', '', 'b'], '1$', 2, 1)

call writefile(g:fixture, 'tests/diff/motion_p2.fxt')
quit!
