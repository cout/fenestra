Fenestra
========

Fenestra is a lightweight libretro frontend (like retroarch) that exposes
performance measurements to help reduce jitter.  It is written in
C++17.

Currently, these metrics include timing information for the event loop that
are sent to stdout once per second.  This can be used to diagnose why frame
rate suddenly drops (as opposed to just showing fps, which can tell you when
frame rate drops, but not why).

In the future, fenestra will capture other important metrics such as number of
context switches, how many frames missed the vsync deadline, and how many
milliseconds late those frames were.  These can then be used to adjust
settings to ensure consistent performance.  Metrics will also be written to a
ring buffer instead of to stdout, so that recording metrics will not impact
performance.

Requirements
------------

* GNU make
* GLFW
* libepoxy
* portaudiocpp

Building
--------

To build fenestra:

```
make
```

Configuring
-----------

For now, fenestra is configured by editing `Config.hpp` and recompiling.

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
