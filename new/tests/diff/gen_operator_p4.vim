" P4.1 — operator composition oracle (nvim --headless -u NONE).
" Records, for [count]op[count]motion / doubled-op sequences, WHAT the operator
" covers: vim puts the deleted/yanked text in the unnamed register, and
" getregtype() tells the shape 'V' (linewise) / 'v' (charwise). The buffer is
" compared before/after to detect cancelled sequences (failed motion: vim
" beeps and changes nothing).
"
" Output line format (" || " separated so empty/spaced text is safe):
"   keys || line,col(1-based) || regtype || regtext(\n escaped) || pict
" pict = the STARTING buffer, lines joined by '|' (regtext's newlines become
" the two characters backslash-n).
" Regenerate from new/tests/diff:  nvim --headless -u NONE -S gen_operator_p4.vim
" (nvim -es exits 1 even on success: check that op_p4.fxt was written).
let s:cases = [
  \ ['2d3w',  ['one two three four five six seven eight nine ten'], 1, 1],
  \ ['d2w',   ['one two three four five six seven eight nine ten'], 1, 1],
  \ ['dw',    ['one two three'], 1, 1],
  \ ['dd',    ['aaa', 'bbb', 'ccc'], 2, 1],
  \ ['2dd',   ['aaa', 'bbb', 'ccc', 'ddd'], 2, 1],
  \ ['d2d',   ['aaa', 'bbb', 'ccc', 'ddd'], 2, 1],
  \ ['2d2d',  ['aaa', 'bbb', 'ccc', 'ddd', 'eee', 'fff'], 1, 1],
  \ ['yy',    ['abc', 'def'], 2, 2],
  \ ['2yy',   ['abc', 'def', 'ghi'], 1, 1],
  \ ['cc',    ['    foo'], 1, 7],
  \ ['cw',    ['one two'], 1, 1],
  \ ['cb',    ['one two three'], 1, 9],
  \ ['db',    ['one two three'], 1, 9],
  \ ['de',    ['one two'], 1, 1],
  \ ['d$',    ['abcd'], 1, 2],
  \ ['d0',    ['abcd'], 1, 4],
  \ ['dj',    ['aaa', 'bbb', 'ccc'], 2, 1],
  \ ['dk',    ['aaa', 'bbb', 'ccc'], 2, 1],
  \ ['dG',    ['aaa', 'bbb', 'ccc', 'ddd'], 2, 1],
  \ ['dgg',   ['aaa', 'bbb', 'ccc', 'ddd'], 3, 2],
  \ ['cw',    ['one  two'], 1, 4],
  \ ['cW',    ['one.two three'], 1, 1],
  \ ['yw',    ['one two'], 1, 1],
  \ ['y$',    ['abcd'], 1, 2],
  \ ['dfz',   ['abc def'], 1, 1],
  \ ['dfd',   ['abc def'], 1, 1],
  \ ['dtd',   ['abc def'], 1, 1],
  \ ['dF.',   ['aaaa.bbbb'], 1, 10],
  \ ['dT.',   ['aaaa.bbbb'], 1, 10],
  \ ['d%',    ['if (a) { b; }'], 1, 4],
  \ ['d}',    ['one', '', 'two', 'three'], 1, 1],
  \ ['d{',    ['one', '', 'two', 'three'], 4, 1],
  \ ['d0',    ['abc'], 1, 1],
  \ ['dq',    ['abc'], 1, 1],
  \ ]
let s:out = []
for s:c in s:cases
  enew!
  call setline(1, s:c[1])
  call setreg('"', 'zz-stale')
  call cursor(s:c[2], s:c[3])
  " record the ACTUAL cursor: cursor() clamps a past-EOL column
  let s:start = line('.') . ',' . col('.')
  let s:before = getline(1, '$')
  call feedkeys(s:c[0], 'nx')
  let s:after = getline(1, '$')
  let s:type = '-'
  let s:text = ''
  if s:before != s:after || getreg('"') !=# 'zz-stale'
    let s:type = getregtype('"')
    let s:text = substitute(join(getreg('"', 1, 1), '\n'), "\n", '\\n', 'g')
  endif
  call add(s:out, s:c[0] . ' || ' . s:start . ' || ' . s:type
        \ . ' || ' . s:text . ' || ' . join(s:before, '|'))
endfor
call writefile(s:out, '/home/ratx86/Projects/cpp/noni/new/tests/diff/op_p4.fxt')
qa!
