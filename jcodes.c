
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

KeySym level1[] = {                                                 XK_J,                                       };
KeySym level2[] = {                                     XK_D, XK_F,       XK_K, XK_L,                           };
KeySym level3[] = { XK_W, XK_E, XK_I, XK_O, XK_A, XK_S,                               XK_semicolon, XK_C, XK_M, };

Display *display;
Window window;
XEvent event;
struct item queue[QUEUE_MAX] = {{0}};
int qlen = 0;
int level = 1;

// We receive X errors if we grab keys that are already grabbed.  This is not
// really fatal so we catch them.
int (*original_error_handler)(Display* display, XErrorEvent* error);

#define IN(needle,haystack) find_in_ints((int)(needle),(int*)(haystack),COUNT(haystack))
int find_in_ints(int needle, int *haystack, int nr)
{
  int i;
  for( i=0; i<nr; i++ ) {
    fprintf(stderr, "Comparing: %d ?= %d\n", needle, haystack[i]);
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

  if( level==1 && IN(it->sym, level1) ) {
    fprintf(stderr, "LEVEL1\n");
    for( i=0; i<COUNT(level2); i++ )
      grab(level2[i]);
    level = 2;
  }
  else if( level==2 && IN(it->sym, level2) ) {
    fprintf(stderr, "LEVEL2\n");
    for( i=0; i<COUNT(level3); i++ )
      grab(level3[i]);
    level = 3;
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
