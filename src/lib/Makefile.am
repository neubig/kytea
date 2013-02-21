LLLIBS = liblinear/liblinear.la
KYTCPP = corpus-io-prob.cpp corpus-io-full.cpp corpus-io-part.cpp corpus-io-tokenized.cpp corpus-io-raw.cpp corpus-io.cpp model-io.cpp string-util.cpp kytea-model.cpp kytea-config.cpp kytea-lm.cpp feature-io.cpp dictionary.cpp feature-lookup.cpp kytea.cpp 
# KYTH = kytea.h corpus-io.h model-io.h string-util.h \
#        kytea-model.h kytea-string.h kytea-struct.h dictionary.h general-io.h \
#        kytea-config.h

AM_CPPFLAGS = -I$(srcdir)/../include -DPKGDATADIR='"$(pkgdatadir)"'

SUBDIRS = liblinear

lib_LTLIBRARIES = libkytea.la

libkytea_la_SOURCES = ${KYTCPP}
libkytea_la_LIBADD = ${LLLIBS}
libkytea_la_LDFLAGS = -version-info 0:0:0
