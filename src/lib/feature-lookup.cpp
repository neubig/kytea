
#include "kytea/feature-lookup.h"

using namespace kytea;
using namespace std;

FeatureLookup::~FeatureLookup() {
    if(charDict_) delete charDict_;
    if(typeDict_) delete typeDict_;
    if(dictVector_) delete dictVector_;
}

void FeatureLookup::addNgramScores(const Dictionary<FeatVec> & dict, 
                                   const KyteaString & str,
                                   int window, 
                                   vector<FeatSum> & score) {
    Dictionary<FeatVec>::MatchResult res = dict.match(str);
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
