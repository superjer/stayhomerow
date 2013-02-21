
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define QUEUE_MAX 1000

Display *display;
Window window;
XEvent event;
XEvent fake_event;
XEvent queue[QUEUE_MAX] = {{0}};

// We receive X errors if we grab keys that are already grabbed.  This is not
// really fatal so we catch them.
int (*original_error_handler)(Display* display, XErrorEvent* error);

int HandleError(Display* display, XErrorEvent* error)
{
  if (error->error_code == BadAccess) {
    fprintf(stderr, "Failed to grab key! Another application has already claimed a key I need. Quitting.\n");
    exit(EXIT_FAILURE);
  }
  return original_error_handler(display, error);
}

void grab(KeySym ks)
{
  KeyCode kc = XKeysymToKeycode(display, ks);
  XGrabKey(display, kc, 0, window, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, kc, LockMask, window, True, GrabModeAsync, GrabModeAsync);
}

void ungrab(KeySym ks)
{
  KeyCode kc = XKeysymToKeycode(display, ks);
  XUngrabKey(display, kc, 0, window);
  XUngrabKey(display, kc, LockMask, window);
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

  // Often, some keys are already grabbed, e.g. by the desktop environment.
  // Set an error handler so that we can ignore those.
  original_error_handler = XSetErrorHandler(&HandleError);

  // grab the J key
  grab(XK_J);

  int killcount = 0;

  // Event loop.
  for (;;) {
    if( killcount++ > 100 ) exit(EXIT_SUCCESS);

    XNextEvent(display, &event);

    switch (event.type) {
      case KeyPress:
      case KeyRelease: {
        if (event.xkey.send_event) {
          fprintf(stderr, "SendEvent loop?\n");
          break;
        }

        fprintf(stderr, "Keycode: %d\n", event.xkey.keycode);
        fake_event = event;
        fake_event.xkey.keycode = XKeysymToKeycode(display, XK_dollar);

        break;
      }
      case FocusOut: {
        // nothing to fake?
        if( !fake_event.type )
          break;

        fprintf(stderr, "FocusOut event\n", event.type);

        // Find the focused window and send the buffered events to it.
        int junk;
        XGetInputFocus(display, &fake_event.xkey.window, &junk);
        fake_event.type = KeyPress;
        XSendEvent(display, fake_event.xkey.window, True, 0, &fake_event);
        fake_event.type = KeyRelease;
        XSendEvent(display, fake_event.xkey.window, True, 0, &fake_event);

        fake_event.type = 0;
        ungrab(XK_J);

        break;
      }
      case FocusIn: {
        break;
      }
      default:
        fprintf(stderr, "Unknown event: %d\n", event.type);
        break;
    }
  }
}
