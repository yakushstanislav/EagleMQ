EagleMQ
-------
EagleMQ is an open source, high-performance and lightweight queue manager.

Documentation
-------------
The latest documentation can be found here: http://github.com/yakushstanislav/EagleMQ/tree/unstable/docs

Building
--------
Currently EagleMQ can be compiled and used only on Linux.

The command to build EagleMQ:

    % make

Installing EagleMQ
------------------
To install EagleMQ use:

    % make install

EagleMQ will be installed in /usr/local/bin.

Running EagleMQ
---------------
To run EagleMQ with the default configuration type:

    % cd src
    % ./eaglemq

You can also specify options on the command line. Example:

    % ./eaglemq eaglemq.conf -d -u /tmp/eaglemq --log-file /tmp/eaglemq.log --pid-file /tmp/eaglemq.pid

Memory allocator
----------------
EagleMQ supports 3 memory allocator: libc malloc, tcmalloc, jemalloc.

The default memory allocator libc malloc.

To compile against libc malloc, use:

    % make MALLOC=libc

To compile against tcmalloc, use:

    % make MALLOC=jemalloc

To compile against jemalloc, use:

    % make MALLOC=tcmalloc
