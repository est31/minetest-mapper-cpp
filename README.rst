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

Create installation package(s):

::

    cpack

Cmake variables:
^^^^^^^^^^^^^^^^

ENABLE_SQLITE3:
    Enable sqlite3 backend support (on by default)

ENABLE_LEVELDB:
    Enable leveldb backend support (off by default)

ENABLE_REDIS:
    Enable redis backend support (off by default)

ENABLE_ALL_DATABASES:
    Enable support for all backends (off by default)

CMAKE_BUILD_TYPE:
    Type of build: 'Release' or 'Debug'. Defaults to 'Release'.

CREATE_FLAT_PACKAGE:
    Create a .tar.gz package suitable for installation in a user's private directory.
    The archive will unpack into a single directory, with the mapper's files inside
    (this is the default).

    If off, .tar.gz, .deb and .rpm packages suitable for system-wide installation
    will be created if possible. The tar.gz package will unpack into a directory hierarchy.

    For creation of .deb and .rpm packages, CMAKE_INSTALL_PREFIX must be '/usr'.

    For .deb package creation, dpkg and fakeroot are required.

    For .rpm package creation, rpmbuild is required.

ARCHIVE_PACKAGE_NAME:
    Name of the .zip or .tar.gz package (without extension). This will also be
    the name of the directory into which the archive unpacks.

    Defaults to minetestmapper-<version>-<os-type>

    The name of .deb and .rpm packages are *not* affected by this variable.

Usage
-----

Binary `minetestmapper` has two mandatory paremeters, `-i` (input world path)
and `-o` (output image path).

::

    ./minetestmapper -i ~/.minetest/worlds/my_world/ -o ~/map.png


Parameters
^^^^^^^^^^

version:
    Print version ID of minetestmapper.

colors <file>:
    Filename of the color definition file to use.

    By default, a file 'colors.txt' is used, which may be located:

    * In the directory of the world being mapped

    * In the directory two levels up from the directory of the world being mapped,
      provided that directory contains a file 'minetest.conf'

    * In the user's private directory ($HOME/.minetest)

    * In the system directory correpsonding to the location where minetestmapper
      is installed. Usually, this would be /usr/share/games/minetestmapper/colors.txt
      or /usr/local/share/games/minetestmapper/colors.txt

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

    The colors file can include other colors files using:

::

    @include filename

    Any entries after the inclusion point override entries from the
    included file. Already defined colors can be 'undefined' by specifying
    '-' as color:

::

    default:stone		71 68 67
    # default-colors.txt might override the color of default:stone
    @include default-colors.txt
    # color of default:dirt_with_grass from default-colors.txt is overridden:
    default:dirt_with_grass	82 117 54
    # Color of water is undefined here:
    default:water_source	-
    default:water_flowing	-

bgcolor:
    Background color of image, `--bgcolor #ffffff`

blockcolor:
    Default color of nodes in blocks that are in the database, `--blockcolor #ffffff`

    If a block is in the database, but some of its nodes are not visible (because they are
    air, or 'invalid', the nodes are drawn in this color (as opposed to the background
    color, which shows for blocks that are not in the database)

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

drawalpha[=cumulative|cumulative-darken|average|none]:
    Allow blocks to be drawn with transparency, `--drawalpha=average`

    In cumulative mode, transparency decreases with depth, and darkness of
    the color increases. After a certain number of transparent blocks
    (e.g. water depth), the color will become opaque, and the underlying
    colors will no longer shine through. The height differences *will*
    still be visible though.

    In cumulative-darken mode, after the color becomes opaque, it will gradually
    be darkened to visually simulate the bigger thickness of transparent blocks.
    The downside is that eventually, the color becomes black.

    Cumulative-darken mode makes deeper, but not too deep water look much better.
    Very deep water will become black though.

    In average mode, all transparent colors are averaged. The blocks remain transparent
    infinitely. If no parameter is given to --drawalpha, 'average' is the default.

    'None' disables alpha drawing. This is the same as not using --drawalpha at all.

    For backward compatibility, 'nodarken' is still recognised as alias for 'cumulative';
    'darken' is still recognised as alias for 'cumulative-darken'. Please don't use
    them, they may disappear in the future.

    Note that each of the different modes has a different color definition
    for transparent blocks that looks best. For instance, for water, the following
    are suggested:

    (disabled): 39 66 106 [192 224 - optional: alpha configuration will be ignored]

    cumulative: 78 132 255 64 224

    cumulative-darken: 78 132 255 64 224

    average: 49 82 132 192 224 (look also good with alpha disabled)

drawair:
    Draw air blocks. `--drawair`

    For best results, the air color should be defined as fully transparent
    so that the color of underlying blocks overrides it.

    This option has a significant performance impact.

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

centergeometry  <geometry>:
    (see below, under 'geometry')

cornergeometry  <geometry>:
    (see below, under 'geometry')

geometry <geometry>:
    Limit the part of the world that is included in the map.

    <geometry> has one of the formats:

    <width>x<height>[<+|-xoffset><+|-yoffset>]	(dimensions & corner)

    <xoffset>,<yoffset>+width+height		(corner & dimensions)

    <xcenter>,<ycenter>:widthxheight		(center & dimensions)

    <xcorner1>,<ycorner1>:<xcorner2>,<ycorner2>

    The old/original format is also supported:

    <xoffset>:<yoffset>+width+height		(corner & dimensions)

    For 'cornergeometry', the offsets ([xy]offset or [xy]center) will
    be at the lower-left corner of the image (offsets increase from left
    to right, and from bottom to top).

    For 'centergeometry', the offsets ([xy]offset or [xy]center) will be
    in the center of the image.

    For plain 'geometry', the offsets will be at the corner, or in
    the center, depending on the geometry format.

    If the offsets are not specified (with the first format),
    the map is centered on the center of the world.

    By default, the geometry has pixel granularity, and a map of
    exactly the requested size is generated.

    *Compatibility mode*:

    If the *first* geometry-related option on the command-line
    is `--geometry`, *and* if the old format is used, then for
    compatibility, the old behavior is default instead (i.e.
    block granularity, and a smaller map if possible). Block
    granularity is also enabled when the obsolete (and otherwise
    undocumented) option '--forcegeometry' is found first.

    Examples:

    `--geometry 10x10-5-5`

    `--geometry 100,100:500,1000`

    `--cornergeometry 50x50+100+100`

    `--centergeometry 1100x1300+1000-500`

    `--geometry 1100x1300`

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

    *NOTE*: If this flag is used, and no actual geometry is
    specified, this would result in a maximum-size map (65536
    x 65536), which is currently not possible, and will fail,
    due to a bug in the drawing library.

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

    * tiled maps (in particular slanted straight lines) may look slightly
      skewed, due to the inserted borders.

    * scale markers never align with tile borders, as the borders are
      logically *between* pixels, so they have no actual coordinates.


tileorigin x,y
    Arrange the tiles so that one tile has its bottom-left (i.e. south-west)
    corner at map coordinates x,y.

    (see also `tilecenter`)

tilecenter x,y|map|world
    Arrange the tiles so that one tile has its center at map coordinates x,y.

    If the value 'world' is used, arrange for one tile to have its center
    at the center of the world instead. This is the default for tiles.

    If the value 'map' is used, arrange for one tile to have its center
    at the center of the map instead.

    Examples:

    `--tilecenter -500,-500`

    `--tileorigin 0,0`

    `--tilecenter map`

    `--tilecenter world`

tilebordercolor
    Color of border between tiles, `--tilebordercolor #000000`

draw[map]<figure> "<geometry> <color> [<text>]"
    Draw a geometrical figure on the map, using either world or map
    coordinates.

    NOTE: the quotes around the two or three parameters to these
    options are absolutely required.

    Possible figures: point, line, circle, ellipse, rectangle, text;
    'circle' is an alias for 'ellipse' - it therefore requires
    two dimensions, just like an ellipse.

    Examples:

    `--drawellipse "5x5+2+3 #ff0000"`

    `--drawcircle "4,5:5x4 #ff0000"`

    `--drawline "5x5+8+8 #80ff0000"`

    `--drawline "8,8:12,12 #80ff0000"`

    `--drawmapline "3x5+4+6 #ffff0000"`

    `--drawtext "0,0 #808080 center of the world"

    `--drawmaptext "0,0 #808080 top left of the map"

    Note that specifying an alpha value does not have the expected
    result when drawing an ellipse.

verbose:
    report some useful / interesting information:

    * maximum coordinates of the world

    * world coordinates included the map being generated

    * number of blocks: in the world, and in the map area.

    Using `--verbose=2`, report some more statistics, including:

    * database access statistics.

verbose-search-colors:
    report the location of the colors file(s) that are read.

    With `--verbose-search-colors=2`, report all locations that are being
    considered as well.
