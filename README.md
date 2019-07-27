# The smithlab_cpp library

This library contains code that has been used in the Smith lab for
several years, and that we now depend on for several of our data
analysis tools. Many of those tools use older versions of this source
code in subdirectories of other repos.

## Requirements

- A C++ compiler that knows C++11. The GNU `g++` compiler works well
  for this after version 5.3.
- The GNU Scientific Library, GSL, which is likely already on your
  system, or easily installed through a package manager.
- The [zlib library](https://zlib.net), which we use for I/O of files
  in gzip format. You likely have this on your system.
- Optional: The [HTSLib library](http://htslib.org), which we use for
  I/O of SAM and BAM format files.

## Building and installing the smithlab_cpp library

Assuming you downloaded the release X.X the tarball
`libsmithlab_cpp-X.X.tar.gz`, you would do the following:
```
$ tar -xvf libsmithlab_cpp-X.X.tar.gz
$ cd libsmithlab_cpp-X.X
$ ./configure
$ make
$ make install
```
If you do not want to contaminate your system's directories with
our code, you can modify the 3rd step above to:
```
$ ./configure --prefix=/some/unimportant/directory
```
It is also a very good idea to run the `configure` script as follows:
```
$ ./configure CXXFLAGS='-O3 -Wall'
```
This will make the resulting library code faster and much smaller, as
the defaults compile with `-g` for debugging.

## Using the source directly from the repo

If you clone the repo and attempt to use the source directly, you are
likely to run into more problems than if you use a release. I will do
everything I can to provide support for the releases, but I may not
help if you have problems using the source repo directly.

This README.md file is written just as we are turning `smithlab_cpp`
into a library and not a collection of source files. If you want to
use it the way it has been used from 2010-2019, then you can use the
`original_makefile.mk` which should still be in this repo. Just like
this:
```
$ make -f original_makefile.mk OptionParser.o
g++ -Wall -std=c++11 -c -o OptionParser.o OptionParser.cpp
```
Now that we are using both the Zlib and HTSLib libraries, it is more
important that we have a build system that can check for these. So
this repo no longer contains a `Makefile`.
