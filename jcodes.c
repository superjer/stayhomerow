
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

// We receive X errors if we grab keys that are already grabbed.  This is not
// really fatal so we catch them.
int failed_grab_count = 0;
int (*original_error_handler)(Display* display, XErrorEvent* error);

int HandleError(Display* display, XErrorEvent* error) {
  if (error->error_code == BadAccess) {
    ++failed_grab_count;
    return 0;
  } else {
    return original_error_handler(display, error);
  }
}

int main(int argc, char* argv[]) {

  // Open the display and get the root window.
  Display* display = XOpenDisplay(NULL);

  if (display == NULL) {
    fprintf(stderr, "Couldn't open display.\n");
    return 1;
  }

  Window window = DefaultRootWindow(display);

  XSetWindowAttributes xswa;
  xswa.event_mask = FocusChangeMask;
  XChangeWindowAttributes(display, window, CWEventMask, &xswa);

  // Often, some keys are already grabbed, e.g. by the desktop environment.
  // Set an error handler so that we can ignore those.
  original_error_handler = XSetErrorHandler(&HandleError);

  // Establish grabs to intercept the events we want.
  int keycode = XKeysymToKeycode(display, XK_J);
  XGrabKey(display, keycode, 0, window, True, GrabModeAsync, GrabModeAsync);

  // Make sure all errors have been reported, then print how many errors we saw.
  XSync(display, False);
  if (failed_grab_count != 0) {
    fprintf(stderr, "Failed to grab %d key combinations.\n", failed_grab_count);
    fprintf(stderr,
      "This is probably because some hotkeys are already grabbed by the system.\n"
      "Unfortunately, these system-wide hotkeys cannot be automatically remapped by\n"
      "this tool.  However, you can usually configure them manually.\n");
  }

  XEvent event;
  XEvent fake_event;
  fake_event.type = 0;

  int killcount = 0;

  // Event loop.
  for (;;) {
    if( killcount++ > 100 ) exit(0);

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
