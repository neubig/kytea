# KYTH = kytea.h corpus-io.h model-io.h string-util.h \
#        kytea-model.h kytea-string.h kytea-struct.h dictionary.h general-io.h \
#        kytea-config.h
KYTH = 

AM_CPPFLAGS = -I$(srcdir)/../include -DPKGDATADIR='"$(pkgdatadir)"'

bin_PROGRAMS = api-example

api_example_SOURCES = api-example.cpp ${KYTH}
api_example_LDADD = ../lib/libkytea.la
