" P3.1 word motions oracle — run with:
"   nvim --headless -u NONE -es -S tests/diff/gen_motion_p3.vim
" Same record format as gen_motion.vim (name keys start end | init @ final).
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

" --- ACCEPT: "foo.bar baz" class mix ---
call Run('w_mid_word_class_switch', ['foo.bar baz'], 'w', 1, 2)
call Run('w_from_punct_to_next', ['foo.bar baz'], 'w', 1, 4)
call Run('w_from_blank_midline', ['foo.bar baz'], 'w', 1, 5)
call Run('w_from_word_start', ['foo.bar baz'], 'w', 1, 1)
call Run('w_last_word_of_line_to_next_line', ['foo.bar'], 'w', 1, 5)
call Run('W_skips_over_punct_run', ['foo.bar baz'], 'W', 1, 1)
call Run('W_from_blank', ['foo.bar baz'], 'W', 1, 5)

" --- class details ---
call Run('w_underscore_digit_keyword', ['x_1 foo'], 'w', 1, 1)
call Run('w_to_punct_after_keyword', ['abc)def'], 'w', 1, 3)
call Run('W_no_split_inside_word', ['a...b'], 'W', 1, 1)
call Run('w_indent_then_next_line', [' indented', 'next'], 'w', 1, 3)

" --- end-of-line skip / empty-line boundary (ACCEPT) ---
call Run('w_end_of_line_skip', ['abc def', 'xyz'], 'w', 1, 5)
call Run('w_stops_on_empty_line', ['ab', '', 'cd'], 'w', 1, 1)
call Run('w_double_through_empties', ['ab', '', '', 'cd'], 'ww', 1, 1)
call Run('w_trailing_blank_stops', ['foo', ''], 'ww', 1, 1)
call Run('w_from_empty_line', ['ab', '', 'cd'], 'w', 2, 1)
call Run('W_also_stops_on_empty', ['ab', '', 'cd'], 'w', 1, 2)

" --- counts ---
call Run('w_count3', ['a b c d e f'], '3w', 1, 1)
call Run('W_count2', ['aaa bbb ccc ddd'], '2W', 1, 1)

" --- end-of-buffer behavior ---
call Run('w_on_last_word_of_file', ['abc def'], 'w', 1, 5)
call Run('w_overflow_count', ['a b'], '5w', 1, 1)
call Run('w_on_last_char_of_file', ['x'], 'w', 1, 1)
call Run('W_last_word_of_file', ['aa bb'], 'W', 1, 4)
call Run('w_from_blank_end', ['x', ' '], 'ww', 1, 1)

" --- probes round 2 (merged into the committed fixture) ---
call Run('w_across_leading_indent', ['ab', '   x'], 'w', 1, 2)
call Run('w_blank_line_transit', ['ab', '   ', 'cd'], 'w', 1, 2)
call Run('w_blank_line_transit_2', ['ab', '   ', 'cd'], 'ww', 1, 2)
call Run('W_across_leading_indent', ['ab', '   x'], 'W', 1, 2)
call Run('W_stops_on_empty', ['ab', '', 'cd'], 'W', 1, 2)
call Run('w_from_blankonly_line', ['x', '  ', 'y'], 'w', 2, 1)
call Run('w_lastline_tail_blank', ['x', '  '], 'ww', 1, 1)
call Run('w_trailing_empty_only', ['x', ''], 'w', 2, 1)
call Run('w_single_to_blank_last', ['x', '  '], 'w', 1, 1)
call Run('w_count_over_blank_transit', ['ab', '  ', 'cd'], '2w', 1, 1)

call writefile(g:fixture, 'tests/diff/motion_p3.fxt')
quit!
