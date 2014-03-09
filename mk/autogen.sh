#!/bin/sh

aclocal
libtoolize
autoconf

rm -rf ./aclocal.m4 autom4te.cache

exit 0

