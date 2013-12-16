Start/Stop command line timer
===============================

This timer works like a start/stop timer,
and store every period using json format.

Dependency
------------

    * jansson  (https://github.com/akheron/jansson)
    * pthread

Compilation
------------
    $ make

Usage
---------

    $ ./timer

Use [Enter] to start/stop timer,
[Q/q] to exit.

Every log will be saved in *time.log* (json format)
