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

struct item {
  int exists;
  XEvent ev;
  KeySym sym;
};

KeySym level1[] = { XK_j, };
KeySym level2[] = { XK_k, XK_n, XK_m, };
KeySym levelm[] = { XK_w, XK_a, XK_s, XK_d, XK_h, XK_k, XK_l, XK_q, };
KeySym leveln[] = { XK_8, XK_9, XK_0, XK_u, XK_i, XK_o, XK_p, XK_k, XK_l, XK_semicolon, XK_m, XK_comma, XK_period, XK_slash, XK_space, XK_Alt_R, XK_Super_R, XK_d, XK_q };

Display *display;
Window window;
XEvent event;
struct item queue[QUEUE_MAX] = {{0}};
int qlen = 0;
int level = 1;
int quit = 0;
KeySym seq[3] = {0};

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

void grab(KeySym ks)
{
  KeyCode kc = XKeysymToKeycode(display, ks);
  XGrabKey(display, kc, 0, window, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, kc, Mod2Mask, window, True, GrabModeAsync, GrabModeAsync);
}

void ungrab(KeySym ks)
{
  KeyCode kc = XKeysymToKeycode(display, ks);
  XUngrabKey(display, kc, 0, window);
  XUngrabKey(display, kc, Mod2Mask, window);
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

  struct item *it = queue + qlen++;
  it->exists = 1;
  it->ev = event;
  it->sym = XKeycodeToKeysym(display, event.xkey.keycode, 0);

  fprintf(stderr,
      "      %s event sym '%s', raw: %c\n",
      event2str(it->ev),
      XKeysymToString(it->sym),
      (int)it->sym
  );

  int i;

  #define KEYSWAP(newkeysym,newstate) do{                        \
    release_find = it->ev.xkey.keycode;                          \
    it->ev.xkey.keycode = XKeysymToKeycode(display,(newkeysym)); \
    if( newstate!=AnyModifier )                                  \
      it->ev.xkey.state = newstate;                              \
    release_replace = it->ev.xkey.keycode;                       \
  } while(0)

  #define REMOVE(keysym) do {                          \
    KeyCode kc = XKeysymToKeycode(display,(keysym));   \
    int phase = 1;                                     \
    for( i=0; i<qlen; i++ ) {                          \
      struct item *it = queue + i;                     \
      if( it->sym!=keysym )                            \
        continue;                                      \
      if( phase==1 && it->ev.type==KeyPress ) {        \
        it->exists = 0;                                \
        phase = 2;                                     \
      }                                                \
      else if( phase==2 && it->ev.type==KeyRelease ) { \
        it->exists = 0;                                \
        phase = 3;                                     \
        break;                                         \
      }                                                \
    }                                                  \
    if( phase==2 )                                     \
      if( !release_kill1 )                             \
        release_kill1 = it->ev.xkey.keycode;           \
      else                                             \
        release_kill2 = it->ev.xkey.keycode;           \
  } while(0)

  if( event.type==KeyRelease ) {
    if( release_find==it->ev.xkey.keycode )
    {
      it->ev.xkey.keycode = release_replace;
      release_find = 0;
      release_replace = 0;
    }
    if( release_kill1==it->ev.xkey.keycode )
    {
      it->exists = 0;
      release_kill1 = 0;
    }
    if( release_kill2==it->ev.xkey.keycode )
    {
      it->exists = 0;
      release_kill2 = 0;
    }
  }
  else if( level==1 ) {
    int status = XGrabKeyboard(display, window, 0, GrabModeAsync, GrabModeAsync, CurrentTime);
    fprintf(stderr, "LEVEL1: XGrabKeyboard status: %d\n", status);
    sym1 = XK_j;
    level = 2;
  }
  else if( level==2 ) {
    #define L2CODE(from,to)                                                      \
      ( it->sym==from ) {                                                        \
        KEYSWAP(to,AnyModifier);                                                 \
        REMOVE(sym1);                                                            \
        XUngrabKeyboard(display, CurrentTime);                                   \
        fprintf(stderr, "LEVEL2: XUngrabKeyboard, XK_j " #from " -> " #to "\n"); \
        level = 1;                                                               \
      }

    if( it->sym==XK_q ) {
      REMOVE(sym1);
      REMOVE(it->sym);
      XUngrabKeyboard(display, CurrentTime);
      fprintf(stderr, "LEVEL2: QUIT!\n");
      level = 1;
      quit = 1;
    }
    else if( it->sym==XK_m ) {
      REMOVE(sym1);
      REMOVE(it->sym);
      it->exists = 0; // why doesn't REMOVE get rid of this???? FIXME
      XUngrabKeyboard(display, CurrentTime);
      for( i=0; i<COUNT(levelm); i++ )
        grab(levelm[i]);
      fprintf(stderr, "LEVEL2: MOVE!\n");
      level = 'm';
    }
    else if( it->sym==XK_n ) {
      REMOVE(sym1);
      REMOVE(it->sym);
      it->exists = 0; // why doesn't REMOVE get rid of this???? FIXME
      XUngrabKeyboard(display, CurrentTime);
      for( i=0; i<COUNT(leveln); i++ )
        grab(leveln[i]);
      fprintf(stderr, "LEVEL2: NUMPAD!\n");
      level = 'n';
    }
    else if( it->sym==XK_k ) {
      sym2 = it->sym;
      level = 3;
    }
    else if L2CODE(XK_j, XK_j         )
    else if L2CODE(XK_w, XK_Page_Up   )
    else if L2CODE(XK_d, XK_BackSpace )
    else if L2CODE(XK_f, XK_Escape    )
    //                g: repeat
    else if L2CODE(XK_h, XK_Home      )
    else if L2CODE(XK_l, XK_End       )
    else if L2CODE(XK_z, XK_Page_Down )
    else if L2CODE(XK_x, XK_Delete    )
    //                c: control
    //                v: shift
    else {
      XUngrabKeyboard(display, CurrentTime);
      fprintf(stderr, "LEVEL2: XUngrabKeyboard\n");
      level = 1;
    }
  }
  else if( level==3 ) {
    #define L3CODE(from2,from3,to,mask)   \
      ( sym2==from2 && it->sym==from3 ) { \
        KEYSWAP(to,mask);                 \
        REMOVE(sym1);                     \
        REMOVE(sym2);                     \
      }

    // JK -- Symbol and punctuation keys
    if(0) ;
    else if L3CODE(XK_k, XK_q         , XK_quotedbl   , ShiftMask)
    else if L3CODE(XK_k, XK_w         , XK_backslash  ,         0)
    else if L3CODE(XK_k, XK_e         , XK_equal      ,         0)
    else if L3CODE(XK_k, XK_r         , XK_asciicircum, ShiftMask)
    else if L3CODE(XK_k, XK_t         , XK_asciitilde , ShiftMask)
    else if L3CODE(XK_k, XK_o         , XK_parenright , ShiftMask)
    else if L3CODE(XK_k, XK_i         , XK_parenleft  , ShiftMask)
    else if L3CODE(XK_k, XK_p         , XK_plus       , ShiftMask)
    else if L3CODE(XK_k, XK_a         , XK_at         , ShiftMask)
    else if L3CODE(XK_k, XK_s         , XK_asterisk   , ShiftMask)
    else if L3CODE(XK_k, XK_d         , XK_dollar     , ShiftMask)
    else if L3CODE(XK_k, XK_f         , XK_Return     , ShiftMask)
    else if L3CODE(XK_k, XK_g         , XK_grave      ,         0)
    else if L3CODE(XK_k, XK_h         , XK_numbersign , ShiftMask)
    else if L3CODE(XK_k, XK_j         , XK_braceleft  , ShiftMask)
    else if L3CODE(XK_k, XK_k         , XK_braceright , ShiftMask)
    else if L3CODE(XK_k, XK_l         , XK_underscore , ShiftMask)
    else if L3CODE(XK_k, XK_semicolon , XK_colon      , ShiftMask)
    else if L3CODE(XK_k, XK_x         , XK_exclam     , ShiftMask)
    else if L3CODE(XK_k, XK_c         , XK_percent    , ShiftMask)
    else if L3CODE(XK_k, XK_v         , XK_bar        , ShiftMask)
    else if L3CODE(XK_k, XK_n         , XK_ampersand  , ShiftMask)
    else if L3CODE(XK_k, XK_m         , XK_minus      ,         0)
    else if L3CODE(XK_k, XK_comma     , XK_greater    , ShiftMask)
    else if L3CODE(XK_k, XK_period    , XK_less       , ShiftMask)

    XUngrabKeyboard(display, CurrentTime);
    fprintf(stderr, "LEVEL3: XUngrabKeyboard\n");
    level = 1;
  }
  else if( level=='m' )
  {
    if( it->sym==XK_q ) {
      REMOVE(it->sym);
      for( i=0; i<COUNT(levelm); i++ )
        ungrab(levelm[i]);
      fprintf(stderr, "LEVELM: XUngrab H,K,L,Q\n");
      level = 1;
    }
    else if( it->sym==XK_w ) KEYSWAP(XK_Up   ,0);
    else if( it->sym==XK_a ) KEYSWAP(XK_Left ,0);
    else if( it->sym==XK_s ) KEYSWAP(XK_Down ,0);
    else if( it->sym==XK_d ) KEYSWAP(XK_Right,0);
    else if( it->sym==XK_h ) KEYSWAP(XK_Left ,0);
    else if( it->sym==XK_j ) KEYSWAP(XK_Down ,0);
    else if( it->sym==XK_k ) KEYSWAP(XK_Up   ,0);
    else if( it->sym==XK_l ) KEYSWAP(XK_Right,0);
  }
  else if( level=='n' )
  {
    if( it->sym==XK_q ) {
      REMOVE(it->sym);
      for( i=0; i<COUNT(leveln); i++ )
        ungrab(leveln[i]);
      fprintf(stderr, "LEVELN: XUngrab Numpad keys\n");
      level = 1;
    }
    else if( it->sym==XK_8         ) KEYSWAP(XK_KP_Divide  ,0);
    else if( it->sym==XK_9         ) KEYSWAP(XK_KP_Multiply,0);
    else if( it->sym==XK_0         ) KEYSWAP(XK_KP_Subtract,0);
    else if( it->sym==XK_u         ) KEYSWAP(XK_KP_7       ,ShiftMask);
    else if( it->sym==XK_i         ) KEYSWAP(XK_KP_8       ,ShiftMask);
    else if( it->sym==XK_o         ) KEYSWAP(XK_KP_9       ,ShiftMask);
    else if( it->sym==XK_p         ) KEYSWAP(XK_KP_Add     ,0);
    else if( it->sym==XK_j         ) KEYSWAP(XK_KP_4       ,ShiftMask);
    else if( it->sym==XK_k         ) KEYSWAP(XK_KP_5       ,ShiftMask);
    else if( it->sym==XK_l         ) KEYSWAP(XK_KP_6       ,ShiftMask);
    else if( it->sym==XK_semicolon ) KEYSWAP(XK_KP_Add     ,0);
    else if( it->sym==XK_m         ) KEYSWAP(XK_KP_1       ,ShiftMask);
    else if( it->sym==XK_comma     ) KEYSWAP(XK_KP_2       ,ShiftMask);
    else if( it->sym==XK_period    ) KEYSWAP(XK_KP_3       ,ShiftMask);
    else if( it->sym==XK_slash     ) KEYSWAP(XK_KP_Enter   ,0);
    else if( it->sym==XK_space     ) KEYSWAP(XK_KP_0       ,ShiftMask);
    else if( it->sym==XK_Alt_R     ) KEYSWAP(XK_KP_Decimal ,0);
    else if( it->sym==XK_Super_R   ) KEYSWAP(XK_KP_Enter   ,0);
    else if( it->sym==XK_d         ) KEYSWAP(XK_BackSpace  ,0);
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
    XSendEvent(display, target, True, 0, &it->ev);

    fprintf(stderr,
        " Sent %s event sym '%s', raw: %c\n",
        event2str(it->ev),
        XKeysymToString(it->sym),
        (int)it->sym
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
  else
  {
    fprintf(stderr, "Couldn't register for FocusChange events on root window. Error: %d\n",error);
    exit(EXIT_FAILURE);
  }

  original_error_handler = XSetErrorHandler(&handle_error);

  // grab the level 1 key(s)
  int i;
  for( i=0; i<COUNT(level1); i++ )
    grab(level1[i]);

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
