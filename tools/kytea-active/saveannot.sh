#!/bin/bash
# set -e

############################
## word-based annotation ##
############################

# find the last saved data
for f in work/*.annot; do curr=`basename $f .annot`; done
next=$((10#$curr+1));
next=`printf "%03d" $next`

if [[ -f save/$next.wann ]]; then
    echo "save/$next.wann already exists!"
else
    unann=`grep -c '!' work/$curr.annot`;
    if [[ "$unann" != "0" ]]; then
        echo "work/$curr.annot still contains $unann unannotated spaces";
    else
        echo "Saving $curr annotation to $next";
        echo "script/merge-annot.pl save/$curr.wann work/$curr.annot > save/$next.wann"
        script/merge-annot.pl save/$curr.wann work/$curr.annot > save/$next.wann
        chmod 444 save/$next.wann
    fi
    [[ -e work/$curr.wordprob ]] && gzip work/$curr.wordprob
fi
