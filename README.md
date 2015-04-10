stayhomerow
===========

Key sequences for X11 that allow you to type difficult keys without leaving the home row.

                      ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───────┐
                      │   │F1 │F2 │F3 │F4 │F5 │F6 │F7 │F8 │F9 │F10│F11│F12│       │
                      ├───┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─────┤
  Tap the Ctrl key    │     │ ( │ ) │   │   │ ~ │   │   │ { │ } │ + │   │   │     │
  to switch the       ├─────┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴─────┤
  keyboard layout     │Esc   │Alt│Shf│Del│ _ │ ` │Hom│PgD│PgU│End│   │   │        │
  to this  ━━▶        ├──────┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴────────┤
                      │Repeat* │ < │ > │Ctl│ | │Bks│N* │M* │   │   │   │          │
                      ├────┬───┴┬──┴─┬─┴───┴───┴───┴───┴───┴──┬┴───┼───┴┬────┬────┤
                      │    │    │    │                        │    │    │    │    │
                      └────┴────┴────┴────────────────────────┴────┴────┴────┴────┘

                        This layout assumes you have Caps remapped to Ctrl in X11.

                        *Repeat works without pressing Ctrl first.
                        *N is for Numpad mode.
                        *M is for Movement mode.


XCAPE
-----

You must install and run XCAPE to get this stuff working:

  <https://github.com/alols/xcape>

Run XCAPE like this:

  xcape -t 250 -e 'Control_L=KP_Multiply;Shift_L=KP_Divide'

This remaps Left Ctrl to numpad  *  and Left Shift to numpad  /, but only when they are tapped
rather quickly. If I was smarter I'd have stayhomerow do this itself. Sorry!


How it works
------------

Unlike other methods, the stayhomerow mappings should work in any application, text-entry or not.
This is because stayhomerow buffers your keystrokes after tapping  Ctrl  and only sends the
synthetic keypress once you've fully specified what you want.

Unfortunately X11 is not designed for any of this, as far as I can tell. You will notice the focus
temporarily leaving your current application while using a mapping, due to X11 limitations. Also,
holding down a key and letting it repeat works, but you won't see anything until you let go.

Some applications (e.g. Firefox) will not accept synthetic keypresses when they are out of focus,
so to be safe, stayhomerow only sends keypress events to the target application after the target
application regains focus. This should happen immediately when you press the final key in the
sequence.


Main sequences
--------------

One of the most useful mappings is  L,  which simulates pressing the End key. The End key,
while freuquently useful, is quite a reach from the QWERTY home row.

    Mapping      Result      Justification
    -------      ------      -------------
    Ctrl, H      Home        H is for Home
    Ctrl, L      End         L is at the End of the homerow
    Ctrl, Ctrl   Esc         Tapping Ctrl twice is easy and fun
    Ctrl, B      Backspace   B is for Backspace
    Ctrl, D      Delete      D is for Delete
    Ctrl, J      Page Down   J means down in HJKL
    Ctrl, K      Page Up     K means up in HJKL


Symbols and punctuation
-----------------------

Some symbols and punctuation could be easier to type, like { curly braces }. Some symbols are
inaccessible on some keyboards, like tilde (~) and grave (`). So let's make them all easier to
get to.

    Mapping      Result      Mnemomic
    -------      ------      --------
    Ctrl, Q      (           (none, these are just neighbors)
    Ctrl, W      )
    Ctrl, T      ~           Tilde
    Ctrl, I      {           In
    Ctrl, O      }           Out
    Ctrl, P      +           Plus
    Ctrl, F      _           Flat
    Ctrl, G      `           Grave
    Ctrl, Z      <           (none, these are just neighbors)
    Ctrl, X      >
    Ctrl, V      |           Vertical bar


Movement mode  [ M ]
--------------------

Tapping  Ctrl  then  M  causes stayhomerow to enter movement mode. Until you press  M,  you can
use the HJKL or WASD keys as arrow keys.

        ┌───┐                ┌───┐
        │ W │                │ ↑ │
      ┌─┴─┬─┴─┬───┐        ┌─┴─┬─┴─┬───┐    ┌───┬───┬───┬───┐       ┌───┬───┬───┬───┐
      │ A │ S │ D │  ━M━▶  │ ← │ ↓ │ → │    │ H │ J │ K │ L │  ━M━▶ │ ← │ ↓ │ ↑ │ → │
      └───┴───┴───┘        └───┴───┴───┘    └───┴─┬─┴─┬─┴───┘       └───┴─┬─┴─┬─┴───┘
                                                  │ M │                   │Quit
All keys other than these nine work normally.     └───┘                   └───┘


Numpad mode  [ N ]
------------------

Tapping  Ctrl  then  N  causes stayhomerow to enter numpad mode. Until you press  N,  the right
hand position mimicks the numeric keypad, as shown below.  You can also press  d  for Backspace
in this mode.

      ┌───┬───┬───┐               ┌───┬───┬───┐
      │ 8 │ 9 │ 0 │               │ / │ * │ - │
    ┌─┴─┬─┴─┬─┴─┬─┴─┐           ┌─┴─┬─┴─┬─┴─┬─┴─┐
    │ U │ I │ O │ P │           │ 7 │ 8 │ 9 │ + │
    └┬──┴┬──┴┬──┴┬──┴┐          └┬──┴┬──┴┬──┴┬──┴┐
     │ J │ K │ L │ ; │    ━N━▶   │ 4 │ 5 │ 6 │ + │
   ┌─┴─┬─┴─┬─┴─┬─┴─┬─┴─┐       ┌─┴─┬─┴─┬─┴─┬─┴─┬─┴─┐
   │ N │ M │ , │ . │ / │       Quit│ 1 │ 2 │ 3 │Enter
  ┈┴───┴───┴──┬┴───┼───┴┐     ┈┴───┴───┴──┬┴───┼───┴┐
              │ Alt│ Win│               0 │  . │Enter
  ┈───────────┴────┴────┘     ┈───────────┴────┴────┘

All keys other than Q, D, and the 18 keys in the diagram work normally.


Control, Shift and Alt  [ C  S  A ]
-----------------------------------

Tapping  Ctrl  then  C  causes the next key to be sent as though the Ctrl  key is held down.
Tapping  Ctrl  then  S  causes the next key to be sent as though the Shift key is held down.
Tapping  Ctrl  then  A  causes the next key to be sent as though the Alt   key is held down.

These can be used together.

Note: Ctrl and Shift stay held down during Movement mode (useful) and Numpad mode (not so useful).


Function keys
-------------

Tap  Ctrl  followed by a number, the  -  key or the  =  key and stayhomerow will send the
corresponding function key, F1-F12. This is especially useful on limited keyboards.


Repeat last key with Shift
--------------------------

Tapping the Shift key re-sends the previously selected key.


Not implemented yet
-------------------

  1. Make the existing sequences work while holding Shift and/or Control, and apply the modifiers
     to the resulting key.
