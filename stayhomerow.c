// stayhomerow
//
// Key sequences for X11 that allow you to type difficult keys without leaving
// the home row.
//
// Copyright 2013 Jer Wilson
// https://github.com/superjer/stayhomerow
// Author: Jer Wilson <superjer@superjer.com>
//
// This program is distributed under the terms of the GNU General Public
// License. See LICENSE for details.
//
// Based very loosely on "Dvorak-Qwerty" by Kenton Varda
// http://dvorak-qwerty.googlecode.com
//
// This program may not have been possible if I hadn't been able to learn how
// to do this sort of thing from the source code of "Dvorak-Qwerty". There is,
// after all, only so much unguided X11 programming a man can take.

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define QUEUE_MAX 1000
#define COUNT(x) (sizeof (x) / sizeof *(x))
#define QUIT_ON_JQ 0

struct item {
  int exists;
  XEvent ev;
  KeySym sym;
};

KeySym movkeys[] = { XK_x, XK_q,
                     XK_w, XK_a, XK_s, XK_d,
                     XK_h, /*j*/ XK_k, XK_l, };

KeySym numkeys[] = { XK_d,     XK_q,
                               XK_8,     XK_9,      XK_0,
                     XK_u,     XK_i,     XK_o,      XK_p,
                     /*j*/     XK_k,     XK_l,      XK_semicolon,
                     XK_m,     XK_comma, XK_period, XK_slash,
                     XK_space, XK_Alt_R, XK_Super_R,              };

Display *display;
Window window;
struct item queue[QUEUE_MAX] = {{0}};
int qlen = 0;
int level = 1;
int quit = 0;
int mask = 0;
KeySym seq[3] = {0};
XEvent last_press_event = {0};

// We receive X errors if we grab keys that are already grabbed.
int (*original_error_handler)(Display* display, XErrorEvent* error);

#define IN(needle,haystack) find_in((needle),(haystack),COUNT(haystack))
int find_in(KeySym needle, KeySym *haystack, int nr)
{
  int i;
  for( i=0; i<nr; i++ ) {
    fprintf(stderr, "Comparing: %c ?= %c\n", (int)needle, (int)haystack[i]);
    if( needle==haystack[i] )
      return 1;
  }
  return 0;
}

int handle_error(Display* display, XErrorEvent* error)
{
  if( error->error_code == BadAccess ) {
    fprintf(stderr, "Failed to grab key! Another application has already claimed a key I need. Quitting.\n");
    exit(EXIT_FAILURE);
  }
  return original_error_handler(display, error);
}

void grab(KeySym ks, int mask)
{
  KeyCode kc = XKeysymToKeycode(display, ks);
  XGrabKey(display, kc, mask         , window, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, kc, mask|Mod2Mask, window, True, GrabModeAsync, GrabModeAsync);
}

void ungrab(KeySym ks,int mask)
{
  KeyCode kc = XKeysymToKeycode(display, ks);
  XUngrabKey(display, kc, mask         , window);
  XUngrabKey(display, kc, mask|Mod2Mask, window);
}

const char *event2str(XEvent ev)
{
  return ev.type==KeyPress ? "KeyPress" : "KeyRelease";
}

void enqueue(XEvent event)
{
  if( qlen >= QUEUE_MAX ) {
    fprintf(stderr, "Queue is full!\n");
    return;
  }

  static KeySym sym1 = 0;
  static KeySym sym2 = 0;
  static KeyCode release_find = 0;
  static KeyCode release_replace = 0;
  static KeyCode release_kill1 = 0;
  static KeyCode release_kill2 = 0;

  int keysyms_per_keycode_return;
  struct item *it = queue + qlen++;
  it->exists = 1;
  it->ev = event;

  KeySym *keysym = XGetKeyboardMapping(display,
      event.xkey.keycode,
      1,
      &keysyms_per_keycode_return);

  it->sym = keysym[0];
  XFree(keysym);

  fprintf(stderr,
      "      %s event sym '%s', raw: %c\n",
      event2str(it->ev),
      XKeysymToString(it->sym),
      (int)it->sym
  );

  int i;

  #define KEYSWAP(newkeysym,newstate) do{                            \
    release_find = it->ev.xkey.keycode;                              \
    it->ev.xkey.keycode = XKeysymToKeycode(display,(newkeysym));     \
    if( newstate!=AnyModifier )                                      \
      it->ev.xkey.state = newstate;                                  \
    release_replace = it->ev.xkey.keycode;                           \
  } while(0)

  #define REMOVE(keysym) do {                                        \
    KeyCode kc = XKeysymToKeycode(display,(keysym));                 \
    int phase = 1;                                                   \
    for( i=0; i<qlen; i++ ) {                                        \
      struct item *it = queue + i;                                   \
      if( it->sym!=keysym )                                          \
        continue;                                                    \
      if( phase==1 && it->ev.type==KeyPress && it->exists ) {        \
        it->exists = 0;                                              \
        phase = 2;                                                   \
      }                                                              \
      else if( phase==2 && it->ev.type==KeyRelease && it->exists ) { \
        it->exists = 0;                                              \
        phase = 3;                                                   \
        break;                                                       \
      }                                                              \
    }                                                                \
    if( phase==2 )                                                   \
      if( !release_kill1 )                                           \
        release_kill1 = it->ev.xkey.keycode;                         \
      else                                                           \
        release_kill2 = it->ev.xkey.keycode;                         \
  } while(0)

  if( event.type==KeyRelease ) {
    if( release_find==it->ev.xkey.keycode ) {
      it->ev.xkey.keycode = release_replace;
      release_find = 0;
      release_replace = 0;
    }
    if( release_kill1==it->ev.xkey.keycode ) {
      it->exists = 0;
      release_kill1 = 0;
    }
    if( release_kill2==it->ev.xkey.keycode ) {
      it->exists = 0;
      release_kill2 = 0;
    }
  }
  else if( level==1 && it->sym==XK_j ) {
    int status = XGrabKeyboard(display, window, 0, GrabModeAsync, GrabModeAsync, CurrentTime);
    fprintf(stderr, "LEVEL1: XGrabKeyboard status: %d\n", status);
    sym1 = it->sym;
    level = 'j';
  }
  else if( level==1 && it->sym==XK_semicolon ) {
    release_find = it->ev.xkey.keycode;
    it->ev.xkey.keycode = last_press_event.xkey.keycode;
    it->ev.xkey.state = last_press_event.xkey.state;
    release_replace = last_press_event.xkey.keycode;
    fprintf(stderr, "LEVEL1: REPEAT! sym:%c state:%x\n", last_press_event.xkey.keycode, last_press_event.xkey.state);
  }
  else if( level==1 ) {
    fprintf(stderr, "LEVEL1: Mysterious sym caught: %d '%c'\n", (int)it->sym, (int)it->sym);
    // FIXME: this can happen when pressing a non-level-1 key while a level-1 key is already down, so we have focus
  }
  else if( level=='j' ) {
    #define LJCODE(from,to)                                                      \
      ( it->sym==from ) {                                                        \
        KEYSWAP(to,mask?:AnyModifier);                                           \
        REMOVE(sym1);                                                            \
        XUngrabKeyboard(display, CurrentTime);                                   \
        fprintf(stderr, "LEVELJ: XUngrabKeyboard, XK_j " #from " -> " #to "\n"); \
        level = 1;                                                               \
        mask = 0;                                                                \
      }

    if( it->sym==XK_q && QUIT_ON_JQ ) {
      REMOVE(sym1);
      REMOVE(it->sym);
      XUngrabKeyboard(display, CurrentTime);
      fprintf(stderr, "LEVELJ: QUIT!\n");
      level = 1;
      quit = 1;
    }
    else if( it->sym==XK_c ) {
      REMOVE(sym1);
      REMOVE(it->sym);
      it->exists = 0; // why doesn't REMOVE get rid of this???? FIXME
      fprintf(stderr, "LEVELJ: CONTROL!\n");
      mask |= ControlMask;
      level = 'c';
    }
    else if( it->sym==XK_v ) {
      REMOVE(sym1);
      REMOVE(it->sym);
      it->exists = 0; // why doesn't REMOVE get rid of this???? FIXME
      fprintf(stderr, "LEVELJ: SHIFT!\n");
      mask |= ShiftMask;
      level = 'v';
    }
    else if( it->sym==XK_m ) {
      REMOVE(sym1);
      REMOVE(it->sym);
      it->exists = 0; // why doesn't REMOVE get rid of this???? FIXME
      XUngrabKeyboard(display, CurrentTime);
      for( i=0; i<COUNT(movkeys); i++ )
        grab(movkeys[i], 0);
      fprintf(stderr, "LEVELJ: MOVE!\n");
      level = 'm';
    }
    else if( it->sym==XK_n ) {
      REMOVE(sym1);
      REMOVE(it->sym);
      it->exists = 0; // why doesn't REMOVE get rid of this???? FIXME
      XUngrabKeyboard(display, CurrentTime);
      for( i=0; i<COUNT(numkeys); i++ )
        grab(numkeys[i], 0);
      fprintf(stderr, "LEVELJ: NUMPAD!\n");
      level = 'n';
    }
    else if( it->sym==XK_k ) {
      sym2 = it->sym;
      level = 'k';
    }
    else if( it->sym==XK_g ) {
      sym2 = it->sym;
      level = 'g';
    }
    else if LJCODE(XK_j, XK_j         )
    else if LJCODE(XK_w, XK_Page_Up   )
    else if LJCODE(XK_d, XK_BackSpace )
    else if LJCODE(XK_f, XK_Escape    )
    //                g: repeat
    else if LJCODE(XK_h, XK_Home      )
    else if LJCODE(XK_l, XK_End       )
    else if LJCODE(XK_z, XK_Page_Down )
    else if LJCODE(XK_x, XK_Delete    )
    else if LJCODE(XK_p, XK_b         )
    else if LJCODE(XK_s, XK_y         )
    else if LJCODE(XK_semicolon, XK_semicolon)
    //                c: control
    //                v: shift
    else {
      XUngrabKeyboard(display, CurrentTime);
      fprintf(stderr, "LEVELJ: XUngrabKeyboard\n");
      level = 1;
      mask = 0;
    }
  }
  else if( level=='k' || level=='g' ) {
    #define LKCODE(from2,from3,to,mask)   \
      ( sym2==from2 && it->sym==from3 ) { \
        KEYSWAP(to,mask);                 \
        REMOVE(sym1);                     \
        REMOVE(sym2);                     \
      }

    // JK -- Symbol and punctuation keys
    if(0) ;
    else if LKCODE(XK_k, XK_q         , XK_quotedbl   , mask|ShiftMask)
    else if LKCODE(XK_k, XK_w         , XK_backslash  , mask          )
    else if LKCODE(XK_k, XK_e         , XK_equal      , mask          )
    else if LKCODE(XK_k, XK_r         , XK_asciicircum, mask|ShiftMask)
    else if LKCODE(XK_k, XK_t         , XK_asciitilde , mask|ShiftMask)
    else if LKCODE(XK_k, XK_o         , XK_parenright , mask|ShiftMask)
    else if LKCODE(XK_k, XK_i         , XK_parenleft  , mask|ShiftMask)
    else if LKCODE(XK_k, XK_p         , XK_plus       , mask|ShiftMask)
    else if LKCODE(XK_k, XK_a         , XK_at         , mask|ShiftMask)
    else if LKCODE(XK_k, XK_s         , XK_asterisk   , mask|ShiftMask)
    else if LKCODE(XK_k, XK_d         , XK_dollar     , mask|ShiftMask)
    else if LKCODE(XK_k, XK_f         , XK_Return     , mask|ShiftMask)
    else if LKCODE(XK_k, XK_g         , XK_grave      , mask          )
    else if LKCODE(XK_k, XK_h         , XK_numbersign , mask|ShiftMask)
    else if LKCODE(XK_k, XK_j         , XK_braceleft  , mask|ShiftMask)
    else if LKCODE(XK_k, XK_k         , XK_braceright , mask|ShiftMask)
    else if LKCODE(XK_k, XK_l         , XK_underscore , mask|ShiftMask)
    else if LKCODE(XK_k, XK_semicolon , XK_colon      , mask|ShiftMask)
    else if LKCODE(XK_k, XK_x         , XK_exclam     , mask|ShiftMask)
    else if LKCODE(XK_k, XK_c         , XK_percent    , mask|ShiftMask)
    else if LKCODE(XK_k, XK_v         , XK_bar        , mask|ShiftMask)
    else if LKCODE(XK_k, XK_n         , XK_ampersand  , mask|ShiftMask)
    else if LKCODE(XK_k, XK_m         , XK_minus      , mask          )
    else if LKCODE(XK_k, XK_comma     , XK_less       , mask          )
    else if LKCODE(XK_k, XK_period    , XK_greater    , mask|ShiftMask)
    else if LKCODE(XK_g, XK_m         , XK_F1         , mask          )
    else if LKCODE(XK_g, XK_comma     , XK_F2         , mask          )
    else if LKCODE(XK_g, XK_period    , XK_F3         , mask          )
    else if LKCODE(XK_g, XK_j         , XK_F4         , mask          )
    else if LKCODE(XK_g, XK_k         , XK_F5         , mask          )
    else if LKCODE(XK_g, XK_l         , XK_F6         , mask          )
    else if LKCODE(XK_g, XK_u         , XK_F7         , mask          )
    else if LKCODE(XK_g, XK_i         , XK_F8         , mask          )
    else if LKCODE(XK_g, XK_o         , XK_F9         , mask          )
    else if LKCODE(XK_g, XK_p         , XK_F10        , mask          )
    else if LKCODE(XK_g, XK_bracketleft , XK_F11      , mask          )
    else if LKCODE(XK_g, XK_bracketright, XK_F12      , mask          )

    XUngrabKeyboard(display, CurrentTime);
    fprintf(stderr, "LEVELK: XUngrabKeyboard\n");
    level = 1;
    mask = 0;
  }
  else if( level=='m' ) {
    if( it->sym==XK_q ) {
      REMOVE(it->sym);
      for( i=0; i<COUNT(movkeys); i++ )
        ungrab(movkeys[i], 0);
      fprintf(stderr, "LEVELM: XUngrab H,K,L,Q\n");
      level = 1;
      mask = 0;
    }
    else if( it->sym==XK_w ) KEYSWAP(XK_Up    ,mask);
    else if( it->sym==XK_a ) KEYSWAP(XK_Left  ,mask);
    else if( it->sym==XK_s ) KEYSWAP(XK_Down  ,mask);
    else if( it->sym==XK_d ) KEYSWAP(XK_Right ,mask);
    else if( it->sym==XK_h ) KEYSWAP(XK_Left  ,mask);
    else if( it->sym==XK_j ) KEYSWAP(XK_Down  ,mask);
    else if( it->sym==XK_k ) KEYSWAP(XK_Up    ,mask);
    else if( it->sym==XK_l ) KEYSWAP(XK_Right ,mask);
    else if( it->sym==XK_x ) KEYSWAP(XK_Delete,mask);
  }
  else if( level=='n' ) {
    if( it->sym==XK_q ) {
      REMOVE(it->sym);
      for( i=0; i<COUNT(numkeys); i++ )
        ungrab(numkeys[i], 0);
      fprintf(stderr, "LEVELN: XUngrab Numpad keys\n");
      level = 1;
    }
    else if( it->sym==XK_8         ) KEYSWAP(XK_KP_Divide  ,mask          );
    else if( it->sym==XK_9         ) KEYSWAP(XK_KP_Multiply,mask          );
    else if( it->sym==XK_0         ) KEYSWAP(XK_KP_Subtract,mask          );
    else if( it->sym==XK_u         ) KEYSWAP(XK_KP_7       ,mask|ShiftMask);
    else if( it->sym==XK_i         ) KEYSWAP(XK_KP_8       ,mask|ShiftMask);
    else if( it->sym==XK_o         ) KEYSWAP(XK_KP_9       ,mask|ShiftMask);
    else if( it->sym==XK_p         ) KEYSWAP(XK_KP_Add     ,mask          );
    else if( it->sym==XK_j         ) KEYSWAP(XK_KP_4       ,mask|ShiftMask);
    else if( it->sym==XK_k         ) KEYSWAP(XK_KP_5       ,mask|ShiftMask);
    else if( it->sym==XK_l         ) KEYSWAP(XK_KP_6       ,mask|ShiftMask);
    else if( it->sym==XK_semicolon ) KEYSWAP(XK_KP_Add     ,mask          );
    else if( it->sym==XK_m         ) KEYSWAP(XK_KP_1       ,mask|ShiftMask);
    else if( it->sym==XK_comma     ) KEYSWAP(XK_KP_2       ,mask|ShiftMask);
    else if( it->sym==XK_period    ) KEYSWAP(XK_KP_3       ,mask|ShiftMask);
    else if( it->sym==XK_slash     ) KEYSWAP(XK_KP_Enter   ,mask          );
    else if( it->sym==XK_space     ) KEYSWAP(XK_KP_0       ,mask|ShiftMask);
    else if( it->sym==XK_Alt_R     ) KEYSWAP(XK_KP_Decimal ,mask          );
    else if( it->sym==XK_Super_R   ) KEYSWAP(XK_KP_Enter   ,mask          );
    else if( it->sym==XK_d         ) KEYSWAP(XK_BackSpace  ,mask          );
    mask = 0;
  }
  else if( level=='c' || level=='v' ) {
    if( it->sym==XK_j ) {
      level = 'j';
      fprintf(stderr, "LEVEL C: starting new sequence, mask:%d\n", mask);
    }
    else {
      it->ev.xkey.state |= mask;
      XUngrabKeyboard(display, CurrentTime);
      fprintf(stderr, "LEVEL C: regular keypress, mask:%d\n", mask);
      level = 1;
      mask = 0;
    }
  }
  else {
    fprintf(stderr, "BAD LEVEL! QUITTING!\n");
    exit(-1);
  }
}

void dequeue()
{
  // Find the focused window and send the queued events to it.
  int i;
  Window target;
  XGetInputFocus(display, &target, &(int){0});

  for( i=0; i<qlen; i++ ) {
    struct item *it = queue + i;
    if( !it->exists )
      continue;
    it->ev.xkey.window = target;
    if( it->ev.type==KeyPress )
      last_press_event = it->ev;
    XSendEvent(display, target, True, 0, &it->ev);

    fprintf(stderr,
        " Sent %s event sym '%s', raw: %c coad: %d\n",
        event2str(it->ev),
        XKeysymToString(it->sym),
        (int)it->sym,
        (int)it->ev.xkey.keycode
    );
  }

  qlen = 0;
}

int main(int argc, char* argv[])
{
  display = XOpenDisplay(NULL);
  if (display == NULL) {
    fprintf(stderr, "Couldn't open the default display.\n");
    exit(EXIT_FAILURE);
  }

  window = DefaultRootWindow(display);

  int error = XChangeWindowAttributes(
      display,
      window,
      CWEventMask,
      &(XSetWindowAttributes){ .event_mask = FocusChangeMask }
  );

  if( error == Success )
    fprintf(stderr, "Succesfully registered for FocusChange events on root window.\n");
  else if( error == BadRequest )
    fprintf(stderr, "Got BadRequest when registering for FocusChange events on root window. Ignoring.\n");
  else {
    fprintf(stderr, "Couldn't register for FocusChange events on root window. Error: %d\n",error);
    exit(EXIT_FAILURE);
  }

  original_error_handler = XSetErrorHandler(&handle_error);

  // grab the level 1 key
  grab(XK_j, 0);
  grab(XK_semicolon, ControlMask);
  XEvent event;

  // Actual title code
  for (;;) {
    XNextEvent(display, &event);

    switch (event.type) {
      case KeyPress:
      case KeyRelease:
        if (!event.xkey.send_event) // avoid event feedback loop
          enqueue(event);
        break;

      case FocusOut:
        if( quit ) exit(0);
        dequeue();
        break;

      case FocusIn:
        break;

      default:
        fprintf(stderr, "Unknown event: %d\n", event.type);
        break;
    }
  }
}
