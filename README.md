Fenestra
========

Fenestra is a lightweight libretro frontend (like retroarch) that exposes
performance measurements to help reduce jitter.  It is written in
C++17.

These metrics include timing measurements for each step through the
event loop, as well as context switches, page faults, audio latency, and
video latency.  Metrics are written in a thread to a binary log file,
which can be visualized in real-time from an out-of-process log viewer.

Requirements
------------

Required:
* GNU make
* pkg-config
* GLFW
* FTGL
* libepoxy
* jsoncpp (for loading config)
* libzstd (for compressing save states)
* libsoil (for loading icons)
* inkscape (for building icons from svg)

Optional:
* alsa
* portaudiocpp
* pulseaudio
* gstreamermm

To install requirements on ubuntu:

```
sudo apt install libjsoncpp-dev pkg-config libftgl-dev libzstd-dev \
                 libepoxy-dev libglfw3-dev libsoil-dev inkscape
```

To install optional dependencies:

```
sudo apt install libpulse-dev
```

Building
--------

To build fenestra:

```
make
```

Configuring
-----------

Fenestra can be configured by editing `fenestra.cfg`.

Button/axies bindings are (for now) configured at compile-time in
`Config.hpp`.

Usage
-----

```
fenestra /path/to/libretro-core.so /path/to/rom
```

Keys
----

The following keys may be used to control fenestra:
* p - pause
* r - reset (2x)
* Esc - exit (2x)

Goals
-----

* Readability - the code should be easy to understand, sometimes at the
  expensive of features
* Consistent performance - input should be read and the game loop should be
  run at the same time each frame, regardless of what might be running on
  other cpus.
* Metrics - good performance metrics can make it easier to diagnose problems,
  even when there is just a brief hiccup in the middle of a game.
* Framebuffer capture - capturing video output should be zero cost (this means
  not capturing using Xcomposite, but instead writing a stream to a ring
  buffer like with SimpleStreamRecorder)
* Autosplitting/tracking - by implementing retroarch network commands,
  fenestra will be usable with any software that supports them (such as
  qusb2snes)
* Audio - latency/stutter should be minimized (this probably menas
  implementing dynamic rate control)

Thanks
------

Fenestra is loosley based on
[nanoarch](https://github.com/heuripedes/nanoarch).  It would not have
been possible to write fenetra without this excellent source as a guide.
