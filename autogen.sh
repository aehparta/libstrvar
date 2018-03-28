#!/bin/bash

set -a

USINGAUTOGENSH=1

# you need to change these values
EMAIL="aehparta@cc.hut.fi"
PACKAGE_NAME="libstrvar"
PACKAGE_DESC="Library to kinda handle variables in 'PHP'-style in C"
PACKAGE_VERSION_MAJOR="0"
PACKAGE_VERSION_MINOR="6"
PACKAGE_VERSION_MICRO="6"
PACKAGE_VERSION="$PACKAGE_VERSION_MAJOR.$PACKAGE_VERSION_MINOR.$PACKAGE_VERSION_MICRO"

# binaries/libraries to install
PACKAGE_BINS=""
PACKAGE_LIBS="libstrvar"

# get build number
PACKAGE_BUILD=`cat debian/build`

# libraries to be checked using pkgconfig
PKGLIBSADD="libddebug"
LIBSADD=""

# include headers when making package
PACKAGE_HEADERS="strvar.h strcalc.h strxml.h strslist.h strhash.h strllist.h strjson.h"


# check pkg-config
if pkg-config --version >/dev/null 2>&1; then
	echo "pkg-config found."
else
	echo "pkg-config not found! you need to install pkg-config"
	exit 1
fi

# check automake version number, 1.7 required
echo "Checking automake version, atleast automake 1.7 needed..."

automake_version="none"
if automake-1.14 --version >/dev/null 2>&1; then
  automake_version="-1.14"
elif automake-1.13 --version >/dev/null 2>&1; then
  automake_version="-1.13"
elif automake-1.12 --version >/dev/null 2>&1; then
  automake_version="-1.12"
elif automake-1.11 --version >/dev/null 2>&1; then
  automake_version="-1.11"
elif automake-1.10 --version >/dev/null 2>&1; then
  automake_version="-1.10"
elif automake-1.9 --version >/dev/null 2>&1; then
  automake_version="-1.9"
elif automake-1.8 --version >/dev/null 2>&1; then
  automake_version="-1.8"
elif automake-1.7 --version >/dev/null 2>&1; then
  automake_version="-1.7"
elif automake-1.6 --version >/dev/null 2>&1; then
  automake_version="-1.6"
elif automake-1.5 --version >/dev/null 2>&1; then
  automake_version="-1.5"
elif automake-1.4 --version >/dev/null 2>&1; then
  automake_version="-1.4"
elif automake --version > /dev/null 2>&1; then
  automake_version=""
  case "`automake --version | sed -e '1s/[^0-9]*//' -e q`" in
	0|0.*|1|1.[0123]|1.[0123][-.]*) automake_version="none" ;;
	1.4*)                             automake_version="-1.4" ;;
	1.5*)                             automake_version="-1.5" ;;
	1.6*)                             automake_version="-1.6" ;;
	1.7*)                             automake_version="-1.7" ;;
	1.8*)                             automake_version="-1.8" ;;
	1.9*)                             automake_version="-1.9" ;;
	1.10*)                             automake_version="-1.10" ;;
	1.11*)                             automake_version="-1.11" ;;
	1.12*)                             automake_version="-1.12" ;;
	1.13*)                             automake_version="-1.13" ;;
	1.14*)                             automake_version="-1.14" ;;
  esac
fi

if test "x${automake_version}" = "xnone"; then
  set +x
  echo "automake not found, you need automake version 1.7 or later."
  exit 1
fi

automake_version_major=`echo "$automake_version" | cut -d. -f2`
automake_version_minor=`echo "$automake_version" | cut -d. -f3`

# need at least automake >= 1.7
if test "$automake_version_major" -lt "7"; then
  echo "automake${automake_version} found."
  echo "this project requires automake >= 1.7.  Please upgrade your version of automake to at least 1.7"
  exit 1
fi

echo "automake${automake_version} found."

echo "Generating configure files... may take a while."

if ! ./configure.ac.sh; then
	echo "Failed to create configure.ac!"
	exit 1
fi

if ! autoreconf --install --force; then
	echo "autoreconf failed!"
	exit 1
fi

ln -s README.md README

case "$1" in
	deb)
		bash ./debian/create-deb-arch.sh
	;;
	
	*)
		echo "now ./configure && make && make install"
	;;
esac
