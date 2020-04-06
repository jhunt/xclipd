#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <X11/Xlib.h>

// A utf-8 buffer containing the last thing pasted
// to the CLIPBOARD X11 selection.
static unsigned char *DATA = NULL;

static Atom CLIPBOARD;
static Atom UTF8_STRING;
static Atom XCLIPD_PROPERTY;
static Atom INCR;

char * now() {
	time_t ts;
	struct tm * tm;
	char *s, *nl;

	time(&ts);
	tm = localtime(&ts);
	if (!tm) {
		return "(time is unknowable)";
	}

	s = asctime(tm);
	nl = strrchr(s, '\n');
	if (nl) {
		*nl = '\0';
	}
	return s;
}

void nab(Display *display, Window w) {
	Window owner;
	XEvent ev;
	XSelectionEvent *sev;

	Atom da, type;
	int di;
	unsigned long size, dul;
	unsigned char *ignored = NULL;

	owner = XGetSelectionOwner(display, CLIPBOARD);
	if (owner == None) {
		fprintf(stderr, "[%s] xclipd: taking ownership of unowned clipboard.\n", now());
		XSetSelectionOwner(display, CLIPBOARD, w, CurrentTime);
		return;
	}

	XConvertSelection(display, CLIPBOARD, UTF8_STRING, XCLIPD_PROPERTY, w, CurrentTime);
	for (;;) {
		XNextEvent(display, &ev);
		switch (ev.type) {
		case SelectionNotify:
			sev = (XSelectionEvent*)&ev.xselection;
			if (sev->property != None) {
				/* Dummy call to get type and size */
				XGetWindowProperty(display, w, XCLIPD_PROPERTY, 0, 0, False, AnyPropertyType,
				                   &type, &di, &dul, &size, &ignored);
				XFree(ignored);
				if (type == INCR) {
					fprintf(stderr, "[%s] xclipd: data too large and INCR mechanism not implemented\n", now());
					return;
				}

				XFree(DATA);
				XGetWindowProperty(display, w, XCLIPD_PROPERTY, 0, size, False, AnyPropertyType,
				                   &da, &di, &dul, &dul, &DATA);

				/* Signal the selection owner that we have successfully read the data. */
				XDeleteProperty(display, w, XCLIPD_PROPERTY);

				fprintf(stderr, "[%s] xclipd: taking on stewardship of %lu bytes on clipboard.\n", now(), strlen((const char *)DATA));
				XSetSelectionOwner(display, CLIPBOARD, w, CurrentTime);
				return;
			} else {
				fprintf(stderr, "[%s] xclipd: selection owner refused our request for clipboard contents!\n", now());
				exit(1);
			}
			break;
		}
	}
}

void deny(Display *display, XSelectionRequestEvent *sev) {
	XSelectionEvent ssev;
	char *an;

	an = XGetAtomName(display, sev->target);
	if (an) {
		XFree(an);
	}

	/* All of these should match the values of the request. */
	ssev.type = SelectionNotify;
	ssev.requestor = sev->requestor;
	ssev.selection = sev->selection;
	ssev.target = sev->target;
	ssev.property = None;  /* signifies "nope" */
	ssev.time = sev->time;

	XSendEvent(display, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

void fulfill(Display *display, XSelectionRequestEvent *sev, Atom UTF8_STRING) {
	XSelectionEvent ssev;
	char *an;

	an = XGetAtomName(display, sev->property);
	if (an) {
		XFree(an);
	}

	XChangeProperty(display, sev->requestor, sev->property, UTF8_STRING, 8, PropModeReplace,
	                DATA, strlen((const char *)DATA));

	ssev.type = SelectionNotify;
	ssev.requestor = sev->requestor;
	ssev.selection = sev->selection;
	ssev.target = sev->target;
	ssev.property = sev->property;
	ssev.time = sev->time;

	XSendEvent(display, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

void bailout() {
	exit(0);
}

int main() {
	Display *display;
	Window target_window;
	int screen;
	XEvent ev;
	XSelectionRequestEvent *sev;
	struct sigaction sa;

	sa.sa_sigaction = bailout;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);

	display = XOpenDisplay(NULL);
	if (!display) {
		fprintf(stderr, "[%s] xclipd: could not open X display\n", now());
		return 1;
	}

	INCR            = XInternAtom(display, "INCR",        False);
	CLIPBOARD       = XInternAtom(display, "CLIPBOARD",   False);
	UTF8_STRING     = XInternAtom(display, "UTF8_STRING", False);
	XCLIPD_PROPERTY = XInternAtom(display, "XCLIPD",      False);

	/* We need a window to receive messages from other clients. */
	screen = DefaultScreen(display);
	target_window = XCreateSimpleWindow(display, RootWindow(display, screen),
	                                    -10, -10, 1, 1, 0, 0, 0);

	nab(display, target_window);

	for (;;) {
		XNextEvent(display, &ev);
		switch (ev.type) {
		case SelectionClear:
			fprintf(stderr, "[%s] xclipd: lost control of the clipboard; regaining...\n", now());
			nab(display, target_window);
			break;

		case SelectionRequest:
			sev = (XSelectionRequestEvent*)&ev.xselectionrequest;
			/* Property is set to None by "obsolete" clients. */
			if (!DATA || sev->target != UTF8_STRING || sev->property == None) {
				fprintf(stderr, "[%s] xclipd: denying (unfulfillable) request for clipboard data\n", now());
				deny(display, sev);
			} else {
				fprintf(stderr, "[%s] xclipd: fulfilling request for clipboard data\n", now());
				fulfill(display, sev, UTF8_STRING);
			}
			break;
		}
	}
}
