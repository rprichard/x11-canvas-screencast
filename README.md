x11-canvas-screencast
=====================

x11-canvas-screencast is an X11-to-HTML5-Canvas screencasting system that uses
the same animation technique as the [anim_encoder][1] project.

[1]: https://www.github.com/sublimehq/anim_encoder

The project includes a Qt-X11 executable, `screencast`, which polls the screen
and mouse cursor and outputs an animation.  Specifically, it writes an
animation script (a CSV-separated list of steps) and a number of PNG files (one
per screen capture and one per unique mouse cursor).  The `pack_animation.py`
script (based on `anim_encoder.py`) optimizes an animation script by creating
a packed PNG file and then replacing each `screen` command with a `blit`
command that copies part of the packed PNG file into the canvas.

Packing the animation can be slow, and this approach makes it easy to view and
tweak the animation prior to packing it.

Prerequisites
-------------

The `screencast` program needs Qt and the XFixes extension.  The
`pack_animation.py` script has the same dependencies as `anim_encoder` --
NumPy, SciPy, OpenCV, and pngcrush.  On Ubuntu, install these packages:

    libqt4-dev libxfixes-dev python-numpy python-scipy python-opencv pngcrush

Usage
-----

1. Build `screencast`:

    <pre>
    $ qmake
    $ make
    </pre>

2. Capture an animation:

    <pre>
    $ ./screencast --rect X Y W H --output example.js
    </pre>

   Hit Enter to stop the capture.

3. Pack the animation:

    <pre>
    $ ./pack_animation.py example.js
    </pre>

   `pack_animation.py example.js` will output `example_packed.js` and
   `example_packed.png`.

Embedding an animation in a web page
------------------------------------

See the [example](example/example.html).

Include the animation's JavaScript file and `player.js` in the page.  The
`player.js` script defines a Player class.  Construct it:

    var player = Player(<animation-script-object>, "<animation-source-dir>");

The `<animation-source-dir>` path will be prefixed to each path in the
animation script.  The `Player` object has an `element` field.  Add or remove
it to a page.  A player is initially paused; unpause it with the `start`
method.  Pause it with the `pause` method.

The `Player` object has two events:

 - `onload`.  This is called after all of the images are loaded, and after
   the player's canvas has been painted with the first frame.

 - `onloop`.  This is called at the end of the animation, as it is looping back
   to the beginning.  Pause it here to prevent looping.

License
-------

BSD license.
