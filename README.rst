Minetest Mapper C++
===================

A port of minetestmapper.py to C++ from https://github.com/minetest/minetest/tree/master/util

Requirements
------------

* libgd
* sqlite3 (enabled by default, set ENABLE_SQLITE3=0 in CMake to disable)
* leveldb (optional, set ENABLE_LEVELDB=1 in CMake to enable leveldb support)
* hiredis (optional, set ENABLE_REDIS=1 in CMake to enable redis support)

Compilation
-----------

Plain:

::

    cmake .
    make

With levelDB and Redis support:

::

    cmake -DENABLE_LEVELDB=true -DENABLE_REDIS=true .
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

colors <file>:
    Filename of the color definition file to use.

    By default, a file 'colors.txt' is used, which may be located:

    * In the directory of the world being mapped

    * In the directory two levels up from the directory of the world being mapped,
      provided that directory contains a file 'minetest.conf'

    * In the user's private directory ($HOME/.minetest)

    * For compatibility, in the current directory as a last resort.
      This causes a warning message to be printed.

    If the colors file contains duplicate entries for the same node,
    one with alpha = 255, or absent, and one with alpha < 255, the former
    is used without 'drawalpha', and the latter is used with 'drawalpha':

::

    # Entry that is used without 'drawalpha':
    default:water-source	39 66 106
    # Entry that is used with 'drawalpha':
    default:water-source	78 132 212 64 224

bgcolor:
    Background color of image, `--bgcolor #ffffff`

scalecolor:
    Color of scale, `--scalecolor #000000`

playercolor:
    Color of player indicators, `--playercolor #ff0000`

    An alpha value can be specified, but due to a bug in the
    drawing library, it will not have the desired effect.

origincolor:
    Color of origin indicator, `--origincolor #ff0000`

    An alpha value can be specified, but due to a bug in the
    drawing library, it will not have the desired effect.

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
    Use specific map backend, supported: auto, sqlite3, leveldb, redis, `--backend leveldb`

    By default, the backend is 'auto', i.e. it is determined from the backend
    setting in the world's world.mt file (if found).

geometry <geometry>:
    (see below, under 'centergeometry')

cornergeometry  <geometry>:
    (see below, under 'centergeometry')

centergeometry  <geometry>:
    Limit the part of the world that is included in the map.

    <geometry> has one of the formats:

    <width>x<height>[<+|-xoffset><+|-yoffset>]

    <xoffset>:<yoffset>+width+height

    For cornergeometry, the offsets will be at the lower-left
    corner of the image (offsets increase from left to right,
    and from bottom to top).

    For centergeometry, the offsets will be in the center of
    the image.

    If the offsets are not specified (with the first format),
    the map is centered on the center of the world.

    By default, the geometry has pixel granularity, and a map of
    exactly the requested size is generated.

    Only if the *first* geometry option on the command-line is
    `--geometry`, then for compatibility, the old behavior
    is default instead (i.e. block granularity, and a smaller
    map if possible). Block granularity is also enabled when
    the obsolete option '--forcegeometry' is found first.

    Examples:

    `--geometry 10x10-5-5`

    `--cornergeometry 50x50+100+100`

    `--centergeometry 1100x1300+1000-500`

    `--centergeometry 1100x1300`

geometrymode pixel,block,fixed,shrink:
    Specify how the geometry should be interpreted. One or
    more of the flags may be used, separated by commas or
    spaces. In case of conflicts, the last flag takes
    precedence.

    When using space as a separator, make sure to enclose
    the list of flags in quotes!

geometrymode pixel:
    Interpret the geometry specification with pixel granularity,
    as opposed to block granularity (see below).

    A map of exactly the requested size is generated (after
    adjustments due to the 'shrink' flag).

geometrymode block:
    Interpret the geometry specification with block granularity.

    The requested geometry will be extended so that the map does
    not contain partial map blocks (of 16x16 nodes each).
    At *least* all pixels covered by the geometry will be in the
    map, but there may be up to 15 more in every direction.

geometrymode fixed:
    Generate a map of the requested geometry, even if part
    or all of it would be empty.

geometrymode shrink:
    Generate a map of at most the requested geometry. Shrink
    it to the smallest possible size that still includes the
    same information.

    Currently, shrinking is done with block granularity, and
    based on which blocks are in the database. If the database
    contains empty, or partially empty blocks, there may still
    be empty pixels at the edges of the map.

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

    Examples:

    `--tiles 1000`

    `--tiles 1000+2`

    NOTE: As a consequence of preserving all map pixels:

    * tiled maps may look slightly distorted, due to the inserted borders.

    * scale markers never align with tile borders, as the borders are
      logically *between* pixels, so they have no actual coordinates.


tileorigin x,y
    Arrange the tiles so that one tile has its bottom-left (i.e. south-west)
    corner at map coordinates x,y.

    By default, tiles are arranged so that one tile has map coordinate 0,0 at
    its center.

    Examples:

    `--tileorigin -500,-500`

    `--tileorigin center-map`

    `--tileorigin center-world`

tilebordercolor
    Color of border between tiles, `--tilebordercolor #000000`

verbose:
    report some useful / interesting information:

    * maximum coordinates of the world

    * world coordinates included the map being generated

    * number of blocks: in the world, and in the map area.

    * database access statistics.

