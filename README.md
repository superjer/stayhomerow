stayhomerow
===========

Key sequence mappings for X11 that allow you to type anything without leaving the home row position.


About
=====

All the sequences in stayhomerow begin with the letter  j,  taking advantage of two things:

  1. The  j  key is the easiest key to press on QWERTY in the home row position, and
  2. English words almost never contain a  j  followed by another consonant.

Unlike other methods, the stayhomerow sequences should work in any application, text-entry or not.
This is because stayhomerow buffers your keystrokes after typing  j  and only sends a synthetic
keypress once you've fully specified what you want.

Unfortunately X11 is not designed for any of this, as far as I can tell. You will notice the focus
temporarily leaving your current application while typing a sequence, due to X11 limitations. Also,
holding down a key and letting it repeat works, but you won't see anything until you let go.

Some applications (e.g. Firefox) will not accept synthetic keypresses when they are out of focus,
so to be safe, stayhomerow only sends keypress events to the target application after the target
application regains focus.


Sequence examples
=================

  Typing  jl  counts as pressing the  Home  key
          jh                          End
          jf                          Esc
          jd                          Backspace
          jx                          Delete
          jw                          Page Up
          jz                          Page Down
  Typing  jq  causes stayhomerow to quit.


Symbols and punctuation examples
================================

Symbols and punctuation sequences all start with  jk.  Some characters are harder than others
to type on QWERTY already, so this is taken into account.  For example, no sequences contain
the  y  or  b  keys. And semicolon has no sequence since it can already be typed easily.

  Typing  jkd  (think Dollar) will type a  $
          jkp         Plus                 +
          jke         Equal                =
          jkl         Lowline              _
          jkg         Grave                `
          jkv         Vertical             |
          jki         In                   (
          jko         Out                  )
          jkj                              {
          jkk                              }

For the full list, see chart.html.


Movement mode
=============

Typing  jm  causes stayhomerow to enter movement mode. Until you press  q,  you can use the
HJKL or WASD keys as arrow keys.


Numpad mode
===========

Typing  jn  causes stayhomerow to enter numpad mode. Until you press  q,  the right hand
position mimicks the numeric keypad, where j=4, k=5, and l=6.  You can also press  d  for
Backspace in this mode.
