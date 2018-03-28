## Makefile.am

AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = testvarlh testvarjson testvarxml

#bin_PROGRAMS = strvar varesimple
#bin_PROGRAMS = varesimple
#strvar_SOURCES = \
#	main.c

#varesimple_SOURCES = \
#	example_simple.c

lib_LTLIBRARIES = libstrvar.la
libstrvar_la_SOURCES = \
	strcalc.c \
	strvar.c \
	strxml.c \
	strhash.c \
	strslist.c \
	strllist.c \
	strjson.c
libstrvar_la_LIBADD = -lm @libddebug_LIBS@
libstrvar_la_CFLAGS = @libddebug_CFLAGS@

testvarlh_SOURCES = test_var_lh.c
testvarlh_CFLAGS = ./.libs/libstrvar.la -lddebug
testvarjson_SOURCES = test_var_json.c
testvarjson_CFLAGS = ./.libs/libstrvar.la -lddebug
testvarxml_SOURCES = test_var_xml.c
testvarxml_CFLAGS = ./.libs/libstrvar.la -lddebug

#strvar_LDFLAGS = ./.libs/libstrvar.a -lm -lpthread
#varesimple_LDFLAGS = ./.libs/libstrvar.a -lm -lpthread
#varesimple_LDFLAGS = -lm -lpthread

include_HEADERS = strvar.h strcalc.h strxml.h strslist.h strhash.h strllist.h strjson.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(PACKAGE_NAME).pc
