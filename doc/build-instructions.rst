Minetest Mapper Build Instructions
##################################

Minetestmapper can be built on posix (linux, freebsd, ...), osx and windows
platforms (although not all platforms receive the same amount of testing -
please let me know if there is a problem on your platform).

Both the gcc and clang compiler suites are supported.

Requirements
============

**Libraries:**

* libgd
* sqlite3 (enabled by default, set ENABLE_SQLITE3=0 in CMake to disable)
* leveldb (optional, set ENABLE_LEVELDB=1 in CMake to enable leveldb support)
* hiredis (optional, set ENABLE_REDIS=1 in CMake to enable redis support)

**Build environment:**

* C++ compiler suite (clang or gcc)
* cmake
* make

**Documentation:**

If converting the documentation to HTML, or another format is desired:

* python-docutils


Compilation
===========

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
----------------

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

    The names of .deb and .rpm packages are *not* affected by this variable.

Converting the documentation
============================

Using python-docutils, the manual can be converted to a variety of formats.

**HTML**

Conversion to HTML yields a nice manual:

::

	cd doc
	rst2html manual.rst > manual.html

**Unix manpage**

Conversion to unix man format has acceptable, but not perfect results:

::

	cd doc
	rst2html manual.rst > minetestmapper.1

**PDF**

The results of using rst2pdf (which, as an aside, is not part of python-docutils)
to convert to PDF directly are not good: random images are scaled down, some even
to almost invisibly small. If PDF is desired, a good option is to open the HTML file
in a webbrowser, and print it to PDF.

**Other**

Other conversions are possible using python-docutils. If you tried any, and
they warrant specific instructions, feel free to contribute.
