" P3.3 f/F/t/T/;/, find-char oracle — run with:
"   nvim --headless -u NONE -es -S tests/diff/gen_motion_p3c.vim
let g:fixture = ['fixture: name keys startLine,startCol endLine,endCol | init @ final']
function! Run(name, init, keys, l, c) abort
  " Space-as-target: '<SPC>' token, the format is space-delimited.
  let realkeys = substitute(a:keys, '<SPC>', ' ', 'g')
  %d _
  for line in a:init
    call append(line('$'), line)
  endfor
  0d _
  call cursor(a:l, a:c)
  call feedkeys(realkeys, "xt")
  let ls = join(getline(1, line('$')), '|')
  let kk = substitute(a:keys, "\e", '<Esc>', 'g')
  call add(g:fixture, printf('%s %s %d,%d %d,%d | %s @ %s',
      \ a:name, escape(kk, '|'), a:l, a:c, line('.'), col('.'),
      \ join(a:init, '|'), ls))
endfunction

" --- f / F in the current line ---
call Run('f_forward_basic', ['axa abc'], 'fa', 1, 2)
call Run('f_count2', ['axa abc'], '2fa', 1, 1)
call Run('f_too_many_fail', ['axa abc'], '3fa', 1, 1)
call Run('f_no_match', ['axa abc'], 'fz', 1, 1)
call Run('f_last_char_no_left', ['abx'], 'fx', 1, 3)
call Run('f_target_space', ['aa bb'], 'f<SPC>', 1, 2)
call Run('F_backward_basic', ['axa abc'], 'Fa', 1, 4)
call Run('F_lexc_cycle', ['axa abc'], 'FaFa', 1, 4)
call Run('F_crosses_blanks', ['ab  a  x'], 'Fa', 1, 9)
call Run('F_space_target', ['a b c'], 'F<SPC>', 1, 5)
call Run('F_count2', ['axa abc'], '2Fa', 1, 5)

" --- t / T: target-adjacent landing ---
call Run('t_before_target', ['axa abc'], 'ta', 1, 1)
call Run('t_fails_no_char', ['abc'], 'tz', 1, 1)
call Run('t_count2', ['axaxx'], '2tx', 1, 1)
call Run('T_after_target', ['axa abc'], 'Ta', 1, 3)
call Run('T_count2', ['axaxx'], '2Tx', 1, 5)

" --- ; and , repeat/reverse ---
call Run('semicolon_repeats_forward', ['xxxy'], 'fx;;', 1, 1)
call Run('semicolon_repeats_backward', ['axa abc'], 'Fa;;', 1, 4)
call Run('comma_reverses_f', ['aba'], 'fa,', 1, 1)
call Run('comma_reverses_t', ['aba'], 'ta,', 1, 1)
call Run('comma_count_after_f', ['abab ab'], 'fb2,', 1, 1)
call Run('semicolon_cross_line', ['ab', 'axab'], 'fbj;', 1, 1)
call Run('semicolon_no_last_find', ['x'], ';', 1, 1)
call Run('t_adjacent_target_stays', ['ax'], 'tx', 1, 1)
call Run('comma_reverses_F', ['aba'], 'Fa,', 1, 3)

" --- failed find STATE semantics: 3fa fails (count), then ';' shows
" whether the failed find still owns the remembered state. Outcomes:
" ';' moves to the next 'a' -> the failed find IS remembered; stays ->
" the old state survived untouched (or none existed).
call Run('countfail_state_probe_fwd', ['axa abc'], '3fa;', 1, 1)
call Run('countfail_state_probe_comma', ['axa abc'], '3Fa,', 1, 3)

call writefile(g:fixture, 'tests/diff/motion_p3c.fxt')
quit!
