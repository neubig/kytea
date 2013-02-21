
#include <kytea/feature-lookup.h>
#include <kytea/kytea-util.h>
#include <kytea/dictionary.h>
#include <algorithm>

using namespace kytea;
using namespace std;

FeatureLookup::~FeatureLookup() {
    if(charDict_) delete charDict_;
    if(typeDict_) delete typeDict_;
    if(selfDict_) delete selfDict_;
    if(dictVector_) delete dictVector_;
    if(biases_) delete biases_;
    if(tagDictVector_) delete tagDictVector_;
    if(tagUnkVector_) delete tagUnkVector_;
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

// Look up values 
void FeatureLookup::addTagNgrams(const KyteaString & chars, 
                                 const Dictionary<FeatVec> * dict, 
                                 vector<FeatSum> & scores,
                                 int window, int startChar, int endChar) {
    if(!dict) return;
    // Create a substring that exactly covers the window that we are interested
    // in of up to -window characters before, and +window characters after
    int myStart = max(startChar-window,0);
    int myEnd = min(endChar+window,(int)chars.length());
    // cerr << "startChar=="<<startChar<<", endChar=="<<endChar<<", myStart=="<<myStart<<", myEnd=="<<myEnd<<endl;
    KyteaString str = 
        chars.substr(myStart, startChar-myStart) +
        chars.substr(endChar, myEnd-endChar);
    // Match the features in this substring
    Dictionary<FeatVec>::MatchResult res = dict->match(str);
    // Add up the sum of all the features
    // myStart-startChar is how far to the left of the starting character we are
    int offset = window-(startChar-myStart);
    for(int i = 0; i < (int)res.size(); i++) {
        // The position we are interested in is the matched position plus the
        // offset
        int pos = res[i].first + offset;
        // Reverse this and multiply by the number of candidates
        pos = (window*2 - pos - 1) * scores.size();
        FeatVal* vec = &((*res[i].second)[pos]);
        // Now add up all the values in the feature vector
        for(int j = 0; j < (int)scores.size(); j++) {
#ifdef KYTEA_SAFE
            if(j+pos >= (int)res[i].second->size() || j+pos < 0)
                THROW_ERROR("j+pos "<<j<<"+"<<pos<<" too big for res[i].second->size() "<<res[i].second->size()<<", window="<<window);
#endif
            scores[j] += vec[j];
        }
    }
}

// Add weights corresponding to the "self" features
// word is the word we are interested in looking up, scores is the output,
// and featIdx is the index of the features
void FeatureLookup::addSelfWeights(const KyteaString & word, 
                                   vector<FeatSum> & scores,
                                   int featIdx) {
#ifdef KYTEA_SAFE
    if(selfDict_ == NULL) THROW_ERROR("Trying to add self weights when no self is present");
#endif
    FeatVec * entry = selfDict_->findEntry(word);
    if(entry) {
        int base = featIdx * scores.size();
        for(int i = 0; i < (int)scores.size(); i++)
            scores[i] += (*entry)[base+i];
    }
}

void FeatureLookup::addDictionaryScores(const Dictionary<ModelTagEntry>::MatchResult & matches, int numDicts, int max, vector<FeatSum> & score) {
    if(dictVector_ == NULL || dictVector_->size() == 0 || matches.size() == 0) return;
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

void FeatureLookup::addTagDictWeights(const std::vector<pair<int,int> > & exists, 
                                      std::vector<FeatSum> & scores) {
    if(!exists.size()) {
        if(tagUnkVector_)
            for(int i = 0; i < (int)scores.size(); i++)
                scores[i] += (*tagUnkVector_)[i];
    } else {
        if(tagDictVector_) {
            int tags = scores.size();
            for(int j = 0; j < (int)exists.size(); j++) {
                int base = exists[j].first*tags*tags+exists[j].second*tags;
                for(int i = 0; i < (int)scores.size(); i++)
                    scores[i] += (*tagDictVector_)[base+i];
            }
        }
    }
}

void FeatureLookup::checkEqual(const FeatureLookup & rhs) const {
    checkPointerEqual(charDict_, rhs.charDict_);
    checkPointerEqual(typeDict_, rhs.typeDict_);
    checkPointerEqual(selfDict_, rhs.selfDict_);
    checkValueVecEqual(dictVector_, rhs.dictVector_);
    checkValueVecEqual(biases_, rhs.biases_);
    checkValueVecEqual(tagDictVector_, rhs.tagDictVector_);
    checkValueVecEqual(tagUnkVector_, rhs.tagUnkVector_);
}
