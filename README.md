stayhomerow
===========

Key sequences for X11 that allow you to type difficult keys without leaving the home row.


About
-----

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
application regains focus. This should happen immediately when you press the final key in the
sequence.


Main sequences  [ j ]
---------------------

One of the most useful sequences is  jl  (JL),  which simulates pressing the End key. The End key,
while freuquently useful, is quite a reach from the QWERTY home row.

    Seq    Result         Justification
    ---    ------         -------------
    jh     Home           H is for Home, and typing j then h is a leftward movement
    jl     End            L is at the End of the letters, and typing it is a rightward movement
    jf     Esc            F is shaped a bit like E? It's also extremely easy to type
    jd     Backspace      D is for delete, which is what Backspace does
    jx     Delete         Because backspace is more important and D is easier to type
    jw     Page Up        W is up from homerow
    jz     Page Down      Z is down from homerow and directly below W
    jj     j              Sometimes you just need one j

Note: Typing  jq  causes stayhomerow to quit.

Note: Type  jj  to simulate a single j, in case you need to literally type  jx,  or similar.


Symbols and punctuation  [ jk ]
-------------------------------

Symbols and punctuation sequences all start with  jk. This is mostly because  jk  is very easy to
type. Some characters are harder than others to type on QWERTY already, so this is taken into
account. For example, no sequences contain the  y  or  b  keys. And semicolon has no sequence since
it can already be typed easily.

    Seq    Result         Mnemomic
    ---    ------         --------
    jkd    $              Dollar
    jkp    +              Plus
    jkm    -              Minus
    jke    =              Equal
    jkl    _              Low line (makes_typing_snake_case_easy_and_fun)
    jkg    `              Grave
    jkv    |              Vertical bar
    jks    *              Star
    jkr    ^              Roof
    jkn    &              aNd
    jki    (              In
    jko    )              Out
    jkj    {              (none, but it's on the left of JK)
    jkk    }              (none, but it's on the right)

For the full list, see chart.html.


Movement mode  [ jm ]
---------------------

Typing  jm  causes stayhomerow to enter movement mode. Until you press  q,  you can use the
HJKL or WASD keys as arrow keys.

    ┌───┬───┐            ┌───┬───┐
    │ Q │ W │            Quit│ ↑ │
    └─┬─┴─┬─┴─┬───┐      └─┬─┴─┬─┴─┬───┐    ┌───┬───┬───┬───┐       ┌───┬───┬───┬───┐
      │ A │ S │ D │ ━jm━▶  │ ← │ ↓ │ → │    │ H │ J │ K │ L │ ━jm━▶ │ ← │ ↓ │ ↑ │ → │
      └───┴───┴───┘        └───┴───┴───┘    └───┴───┴───┴───┘       └───┴───┴───┴───┘

All keys other than these nine work normally.


Numpad mode  [ jn ]
-------------------

Typing  jn  causes stayhomerow to enter numpad mode. Until you press  q,  the right hand
position mimicks the numeric keypad, as shown below.  You can also press  d  for Backspace
in this mode.

      ┌───┬───┬───┐               ┌───┬───┬───┐
      │ 8 │ 9 │ 0 │               │ / │ * │ - │
    ┌─┴─┬─┴─┬─┴─┬─┴─┐           ┌─┴─┬─┴─┬─┴─┬─┴─┐
    │ U │ I │ O │ P │           │ 7 │ 8 │ 9 │ + │
    └┬──┴┬──┴┬──┴┬──┴┐          └┬──┴┬──┴┬──┴┬──┴┐
     │ J │ K │ L │ ; │   ━jn━▶   │ 4 │ 5 │ 6 │ + │
     └─┬─┴─┬─┴─┬─┴─┬─┴─┐         └─┬─┴─┬─┴─┬─┴─┬─┴─┐
       │ M │ , │ . │ / │           │ 1 │ 2 │ 3 │ Enter
    ┈──┴───┴──┬┴───┼───┴┐       ┈──┴───┴──┬┴───┼───┴┐
              │ Alt│ Win│               0 │  . │ Enter
    ┈─────────┴────┴────┘       ┈─────────┴────┴────┘

All keys other than Q, D, and the 18 keys in the diagram work normally.


Control and shift  [ jc ]  [ jv ]
---------------------------------

Typing  jc  causes the next key to be sent as though the Ctrl key is held down.

Typing  jv  causes the next key to be sent as though the Shift key is held down.

These can be used together. They can be used with a regular old single key press, or with another
stayhomerow sequence.

    Seq      Result
    ---      ------
    jca      Ctrl-A
    jc<End>  Ctrl-End
    jcjl     Ctrl-End
    jcjkp    Ctrl-+
    jcjkd    Ctrl-$   (It's control-capital-four! Why? Because we can.)
    jva      A
    jv<End>  Shift-End
    jcjvjl   Ctrl-Shift-End

Note: Ctrl and Shift stay held down during Movement mode (useful) and Numpad mode (not so useful).

Not implemented yet
-------------------

  1. Make the existing sequences work while holding Shift and/or Control, and apply the modifiers
     to the resulting key.

  2. Implement  jg  for pressing Function keys. Use the same positions as in Numpad mode for
     F1-F9. For F10, F11, and F12 use 8, 9, and 0 respectively. Unless somebody can think of a
     better idea.

         Seq      Result
         ---      ------
         jgm      F1
         jg,      F2
         jg.      F3
              ...
         jgo      F9
         jg7      F10
         jg8      F11
         jg9      F12
