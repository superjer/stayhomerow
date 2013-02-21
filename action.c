#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

int my_error_handler(Display *display, XErrorEvent *e) {
        char message[255];
        XGetErrorText(display, e->error_code, message, sizeof(message));
        fprintf(stderr, "Xlib error [code: %d]: '%s'\n", e->error_code, message);
        exit(2);
}

int main(int argc, char *argv[]) {
        int error;
        int reason;
        int screen;
        int event_code;
        Display *display;

        display = XkbOpenDisplay(
           NULL, &event_code, &error, NULL, NULL, &reason);

        if (!display) {
                fprintf(stderr, "Cannot open display.\n");
                exit(1);
        }

        XSetErrorHandler(my_error_handler);
        screen = DefaultScreen(display);

        XkbEvent event;
        XkbSelectEvents(display, XkbUseCoreKbd, XkbAllEventsMask, XkbAllEventsMask);

        for( ;; ) {
                XNextEvent(display, &event.core);
                fprintf(stderr, "event received\n");
        }
}
