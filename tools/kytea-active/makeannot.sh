#!/bin/bash
set -e

NUM_ANNOT=100

# make the word-annotated data
for f in save/*.wann; do p=`basename $f .wann`; done
for f in work/*.wordprob; do wp=`basename $f .wordprob`; done

if [[ -f work/$p.annot ]]; then
    echo "work/$p.annot already exists!"
else
    echo "script/create-annot-word.pl save/$p.wann work/$wp.wordprob $NUM_ANNOT > work/$p.annot"
    script/create-annot-word.pl save/$p.wann work/$wp.wordprob $NUM_ANNOT > work/$p.annot
fi
