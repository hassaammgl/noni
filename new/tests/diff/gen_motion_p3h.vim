" P3.8 curswant oracle: '$' sets vim's MAXCOL aim, vertical moves (j/k, gg/G,
" C-d/C-f/C-u, zt/zb) keep the aim and clamp it into the target line, and
" every horizontal move re-anchors it. Run with:
"   nvim --headless -u NONE -es -S tests/diff/gen_motion_p3h.vim
"
" View state is deterministic the same way as gen_motion_p3f.vim: topline is
" pinned to the case's start line on the last visible row, &scroll is forced
" back to half a page (set lines=24 resets it).
"
" Record format: name keys startLine,startCol endLine,endCol top=<end topline>
" | init @ final   (same parser as the other fixtures; lines are ASCII-only
" because col('.') is a byte column and the replay parser reads codepoints.)
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

" --- plain long -> short -> long column memory ----------------------------
let L = ['abcdefghij', 'ab', 'abcdefghij']
call Run('long_short_long_jj', L, 'jj', 1, 8)
call Run('long_short_long_kk', L, 'kk', 3, 8)
call Run('long_short_long_jjkk', L, 'jjkk', 1, 8)
call Run('short_mid_j', L, 'j', 2, 2)
call Run('mid_col_jj', L, 'jj', 1, 5)
call Run('ll_j', L, 'llj', 1, 1)

" --- '$' sets vim MAXCOL (an infinite aim), not the last-char column ------
call Run('dollar_j', L, '$j', 1, 1)
call Run('dollar_jj', L, '$jj', 1, 1)
call Run('dollar_k', L, '$k', 3, 1)
call Run('dollar_from_short', ['ab', 'abcdefghij'], '$j', 1, 1)
call Run('dollar_already_eol', ['ab', 'abcdefghij'], '$j', 1, 2)
call Run('dollar_empty_line', ['', 'abcdefghij'], '$j', 1, 1)
call Run('dollar_2dollar_j', L, '2$j', 1, 1)
call Run('dollar_j_h_k', L, '$jhk', 1, 1)

" --- horizontal moves re-anchor the aim ----------------------------------
call Run('dollar_0_j', L, '$0j', 1, 1)
call Run('dollar_caret_j', ['  abcdefghij', 'abcdefghij'], '$^j', 1, 1)
call Run('dollar_h_j', ['abcdefghij', 'abcdefghij'], '$hj', 1, 1)
call Run('dollar_w_j', ['abcdefghij', 'abcdefghij', 'abcdefghij'], '$wj', 1, 1)
call Run('dollar_e_j', ['abcdefghij', 'abcdefghij', 'abcdefghij'], '$ej', 1, 1)
call Run('dollar_f_j', ['abcdefghij', 'abcdefghij'], '$faj', 1, 1)
call Run('dollar_pct_fail_j', ['abcdefghij', 'abcdefghij'], '$%j', 1, 1)

" --- gg/G carry the aim (like j/k) --------------------------------------
call Run('dollar_gg_j', ['abc', 'abcdefghij'], '$ggj', 2, 1)
call Run('dollar_G', ['abc', 'abcdefghij'], '$G', 1, 1)
call Run('dollar_G_k', ['abc', 'abcdefghij'], '$Gk', 1, 1)
call Run('G_col', ['abcdefghij', 'ab', 'abcdefghij'], 'G', 1, 2)
call Run('gg_col', ['abcdefghij', 'ab', 'abcdefghij'], 'gg', 3, 2)

" --- scroll commands keep the aim (startofline is off under -u NONE) -----
let p30 = map(range(1, 30), '"S" . printf("%02d", v:val)')
let p30long = map(range(1, 30), '"abcdefghij"')
call Run('cd_col5', p30long, "\x04", 1, 5)
call Run('cd_col5_j', p30long, "\x04" . 'j', 1, 5)
call Run('cu_col10', p30long, "\x15", 20, 10)
call Run('cf_col5', p30long, "\x06", 1, 5)
call Run('cb_col5', p30long, "\x02", 25, 5)
call Run('dollar_cd', p30long, '$' . "\x04", 1, 1)
call Run('dollar_cd_j', p30long, '$' . "\x04" . 'j', 1, 1)
call Run('dollar_cf', p30long, '$' . "\x06", 1, 1)
call Run('dollar_cu', p30long, '$' . "\x15", 20, 1)
call Run('dollar_zt_j', p30, '$ztj', 20, 1)
call Run('dollar_zb_j', p30, '$zbj', 20, 1)
call Run('dollar_H_j', p30, '$Hj', 20, 1)
call Run('dollar_L_j', p30, '$Lj', 1, 1)

" --- insert mode exit re-anchors the aim --------------------------------
call Run('A_esc_j', ['ab', 'abcdefghij'], "AX\<Esc>j", 1, 1)
call Run('i_esc_jj', L, "i\<Esc>jj", 1, 6)

call writefile(g:fixture, 'tests/diff/motion_p3h.fxt')
quit!
