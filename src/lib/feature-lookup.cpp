
#include "kytea/feature-lookup.h"
#include <algorithm>

using namespace kytea;
using namespace std;

FeatureLookup::~FeatureLookup() {
    if(charDict_) delete charDict_;
    if(typeDict_) delete typeDict_;
    if(dictVector_) delete dictVector_;
}

void FeatureLookup::addNgramScores(const Dictionary<FeatVec> * dict, 
                                   const KyteaString & str,
                                   int window, 
                                   vector<FeatSum> & score) {
    if(!dict) return;
    Dictionary<FeatVec>::MatchResult res = dict->match(str);
    // For every entry
    for(int i = 0; i < (int)res.size(); i++) {
        // Let's say we have a n-gram that matched at position 2
        // The first boundary that can be affected is 2-window
        const int base_pos = res[i].first - window;
        const int start = max(0, -base_pos);
        const int end = min(window*2,(int)score.size()-base_pos);
        const FeatVec & vec = *res[i].second;
        for(int j = start; j < end; j++) {
            // cerr << "adding score[" << base_pos+j << "] += vec["<<j<<"] "<<vec[j]<<endl;
            score[base_pos+j] += vec[j];
        }
    }
}

void FeatureLookup::addDictionaryScores(const Dictionary<ModelTagEntry>::MatchResult & matches, int numDicts, int max, vector<FeatSum> & score) {
    if(!dictVector_ || matches.size() == 0) return;
    const int len = score.size(), dictLen = len*3*max;
    vector<char> on(numDicts*dictLen, 0);
    int end;
    ModelTagEntry* myEntry;
    for(int i = 0; i < (int)matches.size(); i++) {
        end = matches[i].first;
        myEntry = matches[i].second;
        if(myEntry->inDict == 0)
            continue;
        const int wlen = myEntry->word.length();
        const int lablen = min(wlen,max)-1;
        for(int di = 0; ((1 << di) & ~1) <= myEntry->inDict; di++) {
            if(myEntry->isInDict(di)) {
                const int dictOffset = di*dictLen;
                // left value (position end-wlen, type
                if(end >= wlen)
                    on[dictOffset + (end-wlen)*3*max +lablen*3 /*+ 0*/] = 1;
                // middle values
                for(int k = end-wlen+1; k < end; k++)
                    on[dictOffset +     k*3*max      +lablen*3 + 1    ] = 1;
                // right value
                if(end != len)
                    on[dictOffset +     end*3*max    +lablen*3 + 2    ] = 1;
            }
        }
    }
    for(int i = 0; i < len; i++) {
        FeatSum & val = score[i];
        for(int di = 0; di < numDicts; di++) {
            char* myOn = &on[di*dictLen + i*3*max];
            FeatVal* myScore = &(*dictVector_)[3*max*di];
            for(int j = 0; j < 3*max; j++) {
                // cerr << "i="<<i<<", di="<<di<<", j/3="<<j/3<<", j%3="<<j%3<<", myOn="<<(int)myOn[j]<<", myScore="<<myScore[j]<<endl;
                val += myOn[j]*myScore[j];
            }
        }
    }
}
