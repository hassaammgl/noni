" P3.7 '%' oracle: bracket matching (()[]{}), the line-local forward scan for
" a bracket when the cursor is not on one, and [count]% file-percentage jumps.
" Run with:
"   nvim --headless -u NONE -es -S tests/diff/gen_motion_p3g.vim
"
" NOTE: every picture line must be ASCII — fixtures record byte columns
" (col('.')) and the replay parser treats them as codepoint columns, which is
" only the same for ASCII (same rule as the other gen_motion*.vim scripts).
"
" Documented deviations from nvim (ROADMAP P3.7): no comment/string awareness
" (nothing is skipped), no '#if'/'#endif' matching, no 'matchpairs' config —
" only the default ()[]{} pairs.
let g:fixture = ['fixture: name keys startLine,startCol endLine,endCol | init @ final']
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
  call add(g:fixture, printf('%s %s %d,%d %d,%d | %s @ %s',
      \ a:name, escape(a:keys, '|'), a:l, a:c, line('.'), col('.'),
      \ join(a:init, '|'), ls))
endfunction

" --- bracket matching ---
let nest = ['if (a[b] == c) {', '    call(x(1), [2, 3]);', '}']
call Run('paren_open', nest, '%', 1, 4)
call Run('brace_open', nest, '%', 1, 16)
call Run('paren_close', nest, '%', 1, 14)
call Run('brace_close', nest, '%', 3, 1)
call Run('inner_square', nest, '%', 1, 6)
call Run('nested_same', ['((()))'], '%', 1, 2)
call Run('nested_same_close', ['((()))'], '%', 1, 4)
call Run('deep_square', ['[[[[[]]]]]'], '%', 1, 3)
call Run('deep_square_close', ['[[[[[]]]]]'], '%', 1, 8)
call Run('adjacent', ['()'], '%', 1, 1)
call Run('adjacent_close', ['()'], '%', 1, 2)
call Run('across_lines', ['call(', '  arg1,', '  arg2)'], '%', 1, 5)
call Run('brace_across_lines', ['a {', '  b', '}'], '%', 1, 3)
call Run('scan_fwd', ['foo(bar)'], '%', 1, 1)
call Run('scan_fwd2', ['a(b)c[d]'], '%', 1, 1)
call Run('scan_on_square', ['a(b)c[d]'], '%', 1, 6)
call Run('scan_from_mid', ['a(b)c'], '%', 1, 3)
call Run('string_plain', ['x = "(a"', 'y = b)'], '%', 1, 6)
call Run('unmatched_open', ['(abc'], '%', 1, 1)
call Run('unmatched_close', ['abc)'], '%', 1, 4)
call Run('mismatch', ['(]'], '%', 1, 1)
call Run('no_bracket', ['plain text'], '%', 1, 3)
call Run('mixed_nest', ['([)]'], '%', 1, 1)
call Run('mixed_nest2', ['(a]b)'], '%', 1, 1)
call Run('empty_line', [''], '%', 1, 1)

" --- [count]% : file percentage (col preserved, clamped) ---
let p40 = map(range(1, 40), '"L" . printf("%02d", v:val)')
call Run('pct_50', p40, '50%', 1, 1)
call Run('pct_25', p40, '25%', 1, 1)
call Run('pct_33', p40, '33%', 1, 1)
call Run('pct_1', p40, '1%', 20, 1)
call Run('pct_99', p40, '99%', 1, 1)
call Run('pct_100', p40, '100%', 1, 1)
call Run('pct_101_noop', p40, '101%', 20, 3)
call Run('pct_999_noop', p40, '999%', 20, 3)
let seven = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
call Run('pct7_30', seven, '30%', 1, 1)
call Run('pct7_50', seven, '50%', 1, 1)
call Run('pct7_1', seven, '1%', 5, 1)
call Run('pct7_66', seven, '66%', 1, 1)
call Run('pct7_100', seven, '100%', 1, 1)
call Run('pct_same_line', ['aaa', 'bbbbb', 'cc'], '50%', 2, 4)
call Run('pct_col_kept', ['aaaaaaaa', 'bb', 'cccccccc'], '50%', 1, 7)
call Run('pct_indent', ['  alpha', '    beta', '  gamma'], '50%', 1, 2)
call Run('pct_after_bracket', ['(a)', 'L2', 'L3'], '50%', 1, 2)

call writefile(g:fixture, 'tests/diff/motion_p3g.fxt')
quit!
