model.bin: model.bin.gz
	gzip -dc model.bin.gz > model.bin

all: model.bin

dist_pkgdata_DATA = \
    model.bin
