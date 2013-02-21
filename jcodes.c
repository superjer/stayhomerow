
#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define arraysize(array) (sizeof(array) / sizeof((array)[0]))

// X keycodes corresponding to keys, regardless of layout.
const int kKeycodes[] = {
                                        20, 21,
  24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
   38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
     52, 53, 54, 55, 56, 57, 58, 59, 60, 61
};

const char kQwerty[] =
            "-="
  "qwertyuiop[]"
  "asdfghjkl;'"
   "zxcvbnm,./";

const char kDvorak[] =
            "[]"
  "',.pyfgcrl/="
  "aoeuidhtns-"
   ";qjkxbmwvz";

// The user has their keyboard layout set to Dvorak.  When we get a keycode, we
// map it to a letter acconding to Qwerty, then figure out which keycode would
// map to the same letter in Dvorak.  This tells us what keycode to send to the
// focus window.  For efficiency, we build a lookup table in keycode_mapping.
//
// keycode --qwerty--> letter --reverse-dvorak--> new keycode
int keycode_mapping[256];

void InitKeycodeMapping() {
  int size = arraysize(kKeycodes);

  int dvorak_to_keycode[128];
  memset(dvorak_to_keycode, 0, sizeof(dvorak_to_keycode));

  for (int i = 0; i < size; i++) {
    dvorak_to_keycode[(int) kDvorak[i]] = kKeycodes[i];
  }

  memset(keycode_mapping, 0, sizeof(keycode_mapping));
  for (int i = 0; i < size; i++) {
    assert(dvorak_to_keycode[(int) kQwerty[i]] != 0);
    keycode_mapping[kKeycodes[i]] = dvorak_to_keycode[(int) kQwerty[i]];
  }
}

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
  InitKeycodeMapping();

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
  for (int i = 0; i < arraysize(kKeycodes); i++) {
    XGrabKey(display, kKeycodes[i], 0, window, True,
             GrabModeAsync, GrabModeAsync);
  }

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

        //event.xkey.keycode = 55;
        fprintf(stderr, "Keycode: %d\n", event.xkey.keycode);

        // Find the focused window and send the event to it.
        int junk;
        XGetInputFocus(display, &event.xkey.window, &junk);
        XSendEvent(display, event.xkey.window, True, 0, &event);

        break;
      }
      case FocusOut: {
        fprintf(stderr, "FocusOut event\n", event.type);

        break;
      }
      case FocusIn: {
        fprintf(stderr, "FocusIn event\n", event.type);

        break;
      }
      default:
        fprintf(stderr, "Unknown event: %d\n", event.type);
        break;
    }
  }
}
