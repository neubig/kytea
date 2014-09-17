# KYTH = kytea.h corpus-io.h model-io.h string-util.h \
#        kytea-model.h kytea-string.h kytea-struct.h dictionary.h general-io.h \
#        kytea-config.h
KYTH = test-kytea.h test-analysis.h test-base.h test-corpusio.h \
       test-sentence.h \
       test-corpusio-euc.h \
       test-corpusio-sjis.h

AM_CPPFLAGS = -I$(srcdir)/../include -DPKGDATADIR='"$(pkgdatadir)"'

TESTS = test-kytea

noinst_PROGRAMS = test-kytea

test_kytea_SOURCES = test-kytea.cpp ${KYTH}
test_kytea_LDADD = ../lib/libkytea.la
