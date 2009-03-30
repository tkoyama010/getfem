#!/bin/bash
# http://sources.redhat.com/automake/automake.html#Local-Macros

# when upgrading libtool change :
#   ltmain.sh with /usr/share/libtool/config/ltmain.sh

function die {
      echo "ERROR: $1";
          exit 1
}

libtoolize -f || die "libtoolize failed";
aclocal -I ./m4 || die "aclocal failed";
autoheader || die "autoheader failed";
autoreconf
autoconf || die "autoconf failed";
#pas de ./ devant les noms des makefiles !!!
automake -a -f --gnu `find . -name Makefile.am | sed -e 's@\./\(.*\)\.am@\1@g'` || die "automake failed";
echo "autogen.sh is ok, you can run the ./configure script"
