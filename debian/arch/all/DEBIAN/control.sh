#!/bin/sh
echo "
Source: $PACKAGE_NAME
Version: $PACKAGE_VERSION
Section: libs
Priority: optional
Maintainer: Antti Partanen <aehparta@cc.hut.fi>
Package: $PACKAGE_NAME
Architecture: $PACKAGE_ARCH
Description: $PACKAGE_DESC
Pre-Depends: libddebug
"