" P3.2 b/B/e/E word-motion oracle — run with:
"   nvim --headless -u NONE -es -S tests/diff/gen_motion_p3b.vim
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

" --- b: backward word starts ---
call Run('b_mid_word', ['foo.bar baz'], 'b', 1, 7)
call Run('b_from_word_start', ['foo.bar baz'], 'b', 1, 5)
call Run('b_from_punct_start', ['foo.bar baz'], 'b', 1, 4)
call Run('b_at_buffer_start_stays', ['foo.bar baz'], 'b', 1, 1)
call Run('b_over_blanks', ['ab  cd'], 'b', 1, 5)
call Run('b_line_cross_to_prev', ['first', 'sec'], 'b', 2, 1)
call Run('b_stops_on_empty', ['ab', '', 'cd'], 'b', 3, 1)
call Run('b_from_empty_line', ['ab', '', 'cd'], 'b', 2, 1)
call Run('b_on_blank', ['a  b'], 'b', 1, 2)
call Run('b_count2', ['a b c d'], '2b', 1, 7)
call Run('b_skips_blankonly_line', ['x', '  ', 'y'], 'b', 3, 1)
call Run('b_to_line_start_nonearlier', ['ab', 'cd'], 'b', 2, 1)
call Run('b_from_line_start_own_word', ['first', 'sec'], 'b', 1, 2)

" --- e: forward word ends ---
call Run('e_mid_word', ['foo.bar baz'], 'e', 1, 2)
call Run('e_from_word_end', ['foo.bar baz'], 'e', 1, 3)
call Run('e_from_punct', ['foo.bar baz'], 'e', 1, 4)
call Run('e_from_blank', ['foo.bar baz'], 'e', 1, 8)
call Run('e_from_first_char', ['foo.bar baz'], 'e', 1, 1)
call Run('e_crossing_to_next_line', ['abc def', 'xyz'], 'e', 1, 5)
call Run('e_cross_empty', ['ab', '', 'cd'], 'e', 1, 2)
call Run('e_count2', ['a bc def'], '2e', 1, 1)
call Run('e_punct_word', ['... abc'], 'e', 1, 1)
call Run('e_last_char_of_file_stays', ['x'], 'e', 1, 1)
call Run('e_last_word_of_file', ['aa bb'], 'e', 1, 4)
call Run('e_from_last_char_of_file', ['aa bb'], 'e', 1, 5)
call Run('e_from_empty_line', ['ab', '', 'cd'], 'e', 2, 1)
call Run('e_count_past_eof', ['abc def'], '7e', 1, 1)

" --- B / E: WORD flavors ---
call Run('B_whole_word', ['foo.bar baz'], 'B', 1, 9)
call Run('B_from_middle', ['foo.bar baz'], 'B', 1, 10)
call Run('B_word_within_line', ['xx yy zz'], 'B', 1, 5)
call Run('B_at_buffer_start_stays', ['foo.bar baz'], 'B', 1, 1)
call Run('B_skips_blankonly_line', ['x', '  ', 'y'], 'B', 3, 1)
call Run('B_stops_on_empty', ['ab', '', 'cd'], 'B', 3, 1)
call Run('E_end_of_word', ['foo.bar baz'], 'E', 1, 2)
call Run('E_from_word_end', ['foo.bar baz'], 'E', 1, 7)
call Run('E_last_char_of_file', ['aa bb'], 'E', 1, 4)
call Run('E_from_last_char_stays', ['aa bb'], 'E', 1, 5)
call Run('E_cross_empty', ['ab', '', 'cd'], 'E', 1, 2)

" --- extra edges ---
call Run('e_to_empty_last_line', ['ab', '', ''], 'e', 1, 2)
call Run('e_from_line_end_to_next', ['abc', 'def'], 'e', 1, 3)
call Run('b_from_mid_of_first_line_word', ['abcd ef'], 'b', 1, 2)
call Run('E_mid_word_line_local', ['a...b c'], 'E', 1, 2)

call writefile(g:fixture, 'tests/diff/motion_p3b.fxt')
quit!
