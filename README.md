xclipd - A Lightweight Clipboard Manager for X11
================================================

This is `xclipd`, a small X11 clipboard manager that I wrote for a
very particular purpose: to support copy-and-paste on a Dockerized
Linux laptop.


## The Original Problem

**tl;dr**: X11 requires the application that copied to the
clipboard to live for as long as you want the clipboard data to be
paste-able.  This daemon solves that for short-lived copiers.

**ne;wm (not 't enough; want more)**: I wrote a whole blog post
about this, over at [jameshunt.us][1].


## How It Works

`xclipd` sits in the background, with a single X11 window object,
and waits for an appliation to assert ownership of the `CLIPBOARD`
selection.  At that point it:

  1. Requests the selection from the current CLIPBOARD owner.
  2. Converts that selection into UTF-8 (if possible).
  3. Takes control of the CLIPBOARD selection.
  4. Responds to any requests for copied data.

Repeat until process termination.

Any time an application copies data to the X11 clipboard, `xclipd`
immediately pastes and re-copies that data, so that the original
client can exit without loss of copied data.


## Building It

Before you build it, you will need:

  1. A C compiler.  Tested with gcc and clang.
     (Don't forget to set the `$CC` environment variable)
  2. libc + header files
  3. libX11 + header files

This codebase has a Makefile that any compatible `make` variant
should be able to use:

    $ make
    cc -Wall -Wextra -o xclipd src/xclipd.c -lX11

There is no `make install` - the easiest way to "install" this is
to copy it to a directory in your `$PATH`:

    $ sudo cp xclipd /usr/local/bin

It's probably easier to build the Docker image, assuming you're
hooked up to a Docker daemon:

    $ make docker
    docker build -t docker/xclipd .
    Sending build context to Docker daemon  100.9kB
    Step 1/9 : FROM alpine:3 AS build
    Step 2/9 : RUN apk update && apk add make gcc libc-dev libx11-dev
    Step 4/9 : COPY . .
    Step 5/9 : RUN make  && mv xclipd /usr/bin
    Removing intermediate container 8e31eafad6d6
    Step 6/9 : FROM alpine:3
    Step 7/9 : RUN apk update && apk add libx11 && rm -f /var/cache/apk/*
    Step 8/9 : COPY --from=build /usr/bin/xclipd /usr/bin/xclipd
    Step 9/9 : ENTRYPOINT ["/usr/bin/xclipd"]
    Successfully built 077d159902d5
    Successfully tagged docker/xclipd:latest

Note that the image is tagged locally as `docker/xclipd`.  If
you're going to push this to an upstream registry (like quay.io or
Docker Hub), don't forget to re-tag it.


## Running It

There are no flags, no options, and `$DISPLAY` is the only
environment variable honored:

    $ xclipd

You'll need to arrange for your own process supervision, bootup,
etc.  Honestly, the Docker approach (shown next) is probably
easier and more rock-solid if you've already committed to Docker.


## Running It (in Docker)

    $ docker run --rm -d -u $UID \
             -v /tmp/.X11-unix:/tmp/.X11-unix \
             -e "DISPLAY=unix$DISPLAY" \
             filefrog/xclipd

You're probably best off running this from something like your
`~/.xinitrc`.


[1]: https://jameshunt.us/writings/x11-clipboard-management-foibles.html
