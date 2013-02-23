
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
KeySym level2[] = { XK_d, XK_f, XK_k, XK_l, };
KeySym level3[] = { XK_w, XK_e, XK_i, XK_o, XK_a, XK_s, XK_d, XK_f, XK_k, XK_l, XK_semicolon, XK_c, XK_m, };

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
    if L2CODE(XK_m,XK_Return)
    else if L2CODE(XK_w,XK_BackSpace)
    else if L2CODE(XK_x,XK_Delete)
    else if L2CODE(XK_h,XK_Home)
    else if L2CODE(XK_z,XK_End)
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
    if( sym2==XK_d && it->sym==XK_w ) {
      KEYSWAP(XK_dollar,ShiftMask);
      REMOVE(sym1);
      REMOVE(sym2);
    }
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
