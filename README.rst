Minetest Mapper C++
===================

A port of minetestmapper.py to C++ from https://github.com/minetest/minetest/tree/master/util

Requirements
------------

* libgd
* sqlite3

Compilation
-----------

Plain:

::

    cmake .
    make

With levelDB support:

::

    cmake -DENABLE_LEVELDB=true .
    make

Debug version:

::

    cmake -DCMAKE_BUILD_TYPE:STRING=Debug .
    make

Release version:

::

    cmake -DCMAKE_BUILD_TYPE:STRING=Release .
    make


Usage
-----

Binary `minetestmapper` has two mandatory paremeters, `-i` (input world path)
and `-o` (output image path).

::

    ./minetestmapper -i ~/.minetest/worlds/my_world/ -o ~/map.png


Parameters
^^^^^^^^^^

bgcolor:
    Background color of image, `--bgcolor #ffffff`

scalecolor:
    Color of scale, `--scalecolor #000000`

playercolor:
    Color of player indicators, `--playercolor #ff0000`

origincolor:
    Color of origin indicator, `--origincolor #ff0000`

drawscale:
    Draw tick marks, `--drawscale`

drawplayers:
    Draw player indicators, `--drawplayers`

draworigin:
    Draw origin indicator, `--draworigin`

drawalpha:
    Allow blocks to be drawn with transparency, `--drawalpha`

noshading:
    Don't draw shading on nodes, `--noshading`

min-y:
    Don't draw nodes below this y value, `--min-y -25`

max-y:
    Don't draw nodes above this y value, `--max-y 75`

backend:
    Use specific map backend, supported: sqlite3, leveldb, `--backend leveldb`

geometry:
    Limit area to specific geometry, `--geometry -800:-800+1600+1600`

forcegeometry:
    Generate a map of the requested size, even if the world is smaller.

sqlite-cacheworldrow:
    When using sqlite, read an entire world row at one, instead of reading
    one block at a time.
    This may improve performance when a large percentage of the world is mapped.

tiles <tilesize>[+<border>]
    Divide the map in square tiles of the requested size. A border of the
    requested width (or width 1, of not specfied) is drawn between the tiles.
    In order to preserve all map pixels (and to prevent overwriting them with
    borders), extra pixel rows and columns for the borders are inserted into
    the map.
    In order to allow partial world maps to be combined into larger maps, edge
    borders of the map are always drawn on the same side (left or top). Other
    edges are always border-less.
    `--tiles 1000`
    `--tiles 1000+2`
    NOTE: As a consequence of preserving all map pixels:
    * tiled maps may look slightly distorted, due to the inserted borders.
    * scale markers never align with tile borders, as the borders are
      logically *between* pixels, so they have no actual coordinates.


tileorigin x:y
    Arrange the tiles so that one tile has its bottom-left (i.e. south-west)
    corner at map coordinates x,y.
    By default, tiles are arranged so that one tile has map coordinate 0,0 at
    its center.
    `--tileorigin -500,-500`
    `--tileorigin center-map`
    `--tileorigin center-world`

tilebordercolor
    Color of border between tiles, `--tilebordercolor #000000`

verbose:
    report some useful/ interesting information:
    * maximum coordinates of the world
    * world coordinates included the map being generated
    * number of blocks: in the world, and in the map area.
    * database access statistics.

