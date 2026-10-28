" P3.6 H/M/L + zz/zt/zb + ctrl-scrolls oracle. runes = 22 effective rows at
" the default &lines=24 (nvim headless minus chrome). Run with:
"   nvim --headless -u NONE -es -S tests/diff/gen_motion_p3f.vim
"
" View state is made DETERMINISTIC: after loading the picture the window is
" restored with topline = max(1, l - (rows-1)), i.e. the cursor on the last
" visible row — the state vim itself produces after jumping to that line with
" scrolloff 0. (Plain cursor() lets the previous case's topline leak through
" in --headless, which made half-page cases unreproducible.)
"
" Record format (the extra `top=N` token is ignored by the older parsers):
"   name keys startLine,startCol endLine,endCol top=<end topline> | init @ final
" (topline is 1-based like nvim; 1 = line 0 of the buffer.)
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
  " `set lines=24` recomputes the default 'scroll' (and does so from a
  " half-initialised window), so pin it to the real default: half a page.
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

let p40 = map(range(1, 40), '"L" . printf("%02d", v:val)')

" --- H / M / L (col carried like j/k, vertical) ---
call Run('H_plain', p40, 'H', 10, 2)
call Run('H_count3', p40, '3H', 10, 2)
call Run('M_plain', p40, 'M', 10, 2)
call Run('M_after_scroll', p40, "\x04" . 'M', 10, 2)
call Run('L_plain', p40, 'L', 10, 2)
call Run('L_at_eof_view', p40, 'L', 40, 1)
call Run('L_count2', p40, '2L', 10, 2)
call Run('H_near_eof_view', p40, "\x06" . 'H', 1, 2)

" --- zz zt zb: cursor normally stays; reveal keys pin the new top ---
call Run('zz_cursor_stays', p40, 'zz', 20, 3)
call Run('zz_center_via_L', p40, 'zzL', 20, 3)
call Run('zz_center_via_H', p40, 'zzH', 20, 3)
call Run('zz_near_eof_H', p40, 'zzH', 40, 1)
call Run('zt_via_H', p40, 'ztH', 20, 3)
call Run('zt_at_eof_H', p40, 'ztH', 40, 1)
call Run('zb_via_L', p40, 'zbL', 10, 3)
call Run('zb_at_top_H', p40, 'zbH', 5, 3)
call Run('zt_count_line', p40, '25ztH', 10, 2)
call Run('zz_count_line', p40, '3zzH', 20, 3)
call Run('zb_count_line', p40, '20zbL', 5, 2)

" --- ctrl+f / ctrl+b (page = rows-2, cursor jumps to the window edge) ---
call Run('Cf_plain', p40, "\x06", 1, 3)
call Run('Cf_from_mid', p40, "\x06", 10, 3)
call Run('Cf_eof', p40, "\x06", 30, 2)
call Run('Cf_all_the_way_eof', p40, "\x06\x06\x06", 1, 2)
call Run('Cf_at_eof_noop', p40, "\x06", 40, 1)
call Run('Cb_from_eof', p40, "\x02", 40, 1)
call Run('Cb_via_G', p40, 'G' . "\x02", 1, 1)
call Run('Cb_at_top_noop', p40, "\x02", 5, 2)

" --- ctrl+d / ctrl+u (half = rows/2; count = lines) ---
call Run('Cd_plain', p40, "\x04", 1, 3)
call Run('Cd_from_mid', p40, "\x04", 5, 2)
call Run('Cd_count3', p40, '3' . "\x04", 1, 2)
call Run('Cd_eof', p40, "\x04", 35, 2)
call Run('Cd_at_eof_noop', p40, "\x04", 40, 1)
call Run('Cu_plain', p40, "\x15", 25, 3)
call Run('Cu_top', p40, "\x15", 5, 3)
call Run('Cu_at_top_noop', p40, "\x15", 1, 3)
call Run('Cu_count4', p40, '4' . "\x15", 25, 3)

" --- short buffer / EOF-region / count-overflow edges ---
let p10 = map(range(1, 10), '"S" . printf("%02d", v:val)')
call Run('short_M', p10, 'M', 3, 2)
call Run('short_L', p10, 'L', 3, 2)
call Run('short_Cd', p10, "\x04", 6, 2)
call Run('short_Cf', p10, "\x06", 6, 2)
call Run('short_zzH', p10, 'zzH', 5, 2)
call Run('H_count_over', p40, '30H', 10, 2)
call Run('L_count_over', p40, '30L', 10, 2)
call Run('Cf_count2', p40, '2' . "\x06", 1, 1)
call Run('Cb_count2', p40, '2' . "\x02", 40, 1)

call writefile(g:fixture, 'tests/diff/motion_p3f.fxt')
quit!
