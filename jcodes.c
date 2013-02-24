
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

Display *display;
Window window;
XEvent event;
struct item queue[QUEUE_MAX] = {{0}};
int qlen = 0;
int level = 1;
KeySym seq[3] = {0};

// We receive X errors if we grab keys that are already grabbed.  This is not
// really fatal so we catch them.
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
    for( i=0; i<qlen-1; i++ ) {                        \
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

    if      L2CODE(XK_j, XK_j         )
    //                q: quit
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
    else if( IN(it->sym, level2) ) {
      sym2 = it->sym;
      level = 3;
    }
    else {
      XUngrabKeyboard(display, CurrentTime);
      fprintf(stderr, "LEVEL2: XUngrabKeyboard\n");
      level = 1;
    }
  }
  else {
    #define L3CODE(from2,from3,to,mask)   \
      ( sym2==from2 && it->sym==from3 ) { \
        KEYSWAP(to,mask);                 \
        REMOVE(sym1);                     \
        REMOVE(sym2);                     \
      }

    // JM -- movement
    if      L3CODE(XK_m, XK_h         , XK_Left       , 0)
    else if L3CODE(XK_m, XK_j         , XK_Down       , 0)
    else if L3CODE(XK_m, XK_k         , XK_Up         , 0)
    else if L3CODE(XK_m, XK_l         , XK_Right      , 0)
    // JN -- numpad
    else if L3CODE(XK_n, XK_8         , XK_KP_Divide  , 0)
    else if L3CODE(XK_n, XK_9         , XK_KP_Multiply, 0)
    else if L3CODE(XK_n, XK_0         , XK_KP_Subtract, 0)
    else if L3CODE(XK_n, XK_u         , XK_KP_7       , 0)
    else if L3CODE(XK_n, XK_i         , XK_KP_8       , 0)
    else if L3CODE(XK_n, XK_o         , XK_KP_9       , 0)
    else if L3CODE(XK_n, XK_p         , XK_KP_Add     , 0)
    else if L3CODE(XK_n, XK_j         , XK_KP_4       , 0)
    else if L3CODE(XK_n, XK_k         , XK_KP_5       , 0)
    else if L3CODE(XK_n, XK_l         , XK_KP_6       , 0)
    else if L3CODE(XK_n, XK_semicolon , XK_KP_Add     , 0)
    else if L3CODE(XK_n, XK_m         , XK_KP_1       , 0)
    else if L3CODE(XK_n, XK_comma     , XK_KP_2       , 0)
    else if L3CODE(XK_n, XK_period    , XK_KP_3       , 0)
    else if L3CODE(XK_n, XK_slash     , XK_KP_Enter   , 0)
    else if L3CODE(XK_n, XK_space     , XK_KP_0       , 0)
    else if L3CODE(XK_n, XK_Alt_R     , XK_KP_Decimal , 0)
    else if L3CODE(XK_n, XK_Super_R   , XK_KP_Enter   , 0)
    // JK -- Symbol and punctuation keys
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
    else if L3CODE(XK_k, XK_f         , XK_greater    , ShiftMask)
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
