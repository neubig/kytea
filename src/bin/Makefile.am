# KYTH = kytea.h corpus-io.h model-io.h string-util.h \
#        kytea-model.h kytea-string.h kytea-struct.h dictionary.h general-io.h \
#        kytea-config.h
KYTH = 

AM_CPPFLAGS = -I$(srcdir)/../include -DPKGDATADIR='"$(pkgdatadir)"'

bin_PROGRAMS = kytea train-kytea

kytea_SOURCES = run-kytea.cpp ${KYTH}
kytea_LDADD = ../lib/libkytea.la

train_kytea_SOURCES = train-kytea.cpp ${KYTH}
train_kytea_LDADD = ../lib/libkytea.la
