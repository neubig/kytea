#!/bin/bash
set -e

ORIG_DATA=data/target-train.raw

KYTEA=kytea
TRAIN=train-kytea

GEN_CORPORA="-full data/wiki-sample.word"
DICTS=
#DICTS="-dict data/unidic.word"

TRAIN_OPT="-dictn 4 -charw 3 -charn 3 -typew 3 -typew 3"

################################
## make the word-based model ##
################################

# initialize
if [[ ! -d save ]]; then mkdir save; fi
if [[ ! -d work ]]; then mkdir work; fi
if [[ ! -f save/000.wann ]]; then
    echo "Creating original corpus."
    script/splitchars.pl < $ORIG_DATA > save/000.wann
    chmod 444 save/000.wann
fi

# find the last saved data
for f in save/*.wann; do
    num=`basename $f .wann`
done

# check
if [[ -f work/$num.mod ]]; then
    echo "work/$num.mod already exists!"
else

    # make the model
    echo "$TRAIN -notags $TRAIN_OPT $GEN_CORPORA $DICTS -part save/$num.wann -model work/$num.mod"
    $TRAIN -notags $TRAIN_OPT $GEN_CORPORA $DICTS -part save/$num.wann -model work/$num.mod
     
    # parse the corpus
    echo "$KYTEA -model work/$num.mod -out prob > work/$num.wordprob"
    $KYTEA -model work/$num.mod -out prob < $ORIG_DATA > work/$num.wordprob

fi

./makeannot.sh
