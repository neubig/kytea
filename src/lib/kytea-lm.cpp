#include <kytea/kytea-lm.h>
#include <kytea/kytea-util.h>
#include <iostream>
#include <cmath>

using namespace kytea;
using namespace std;


// increment the count of a probability and return whether the
//  n-gram already exists
bool addCount(KyteaDoubleMap & myMap, const KyteaString & str) {
    KyteaDoubleMap::iterator it = myMap.find(str);
    if(it == myMap.end()) {
        myMap.insert(pair<KyteaString,double>(str,1.0));
        return false;
    } 
    it->second++;
    return true;
}

// train an n-gram model using Kneser-Ney smoothing
void KyteaLM::train(const std::vector<KyteaString> & corpus) {
    // get the marginal counts
    KyteaDoubleMap denominators;
    for(unsigned i = 0; i < corpus.size(); i++) {
        KyteaString trainString(corpus[i].length()+n_);
        for(unsigned j = 0; j < n_-1; j++)
            trainString[j] = 0;
        trainString[trainString.length()-1] = 0;
        trainString.splice(corpus[i],n_-1);
        for(unsigned j = n_; j < trainString.length(); j++) {
            for(unsigned len = n_; len > 0; len--) {
                KyteaString fbString = trainString.substr(j-len,len-1);
                addCount(denominators,fbString);
                // make sure we're getting marginal counts, not actual (Kneser-Ney)
                if(addCount(probs_, trainString.substr(j-len, len)))
                    break;
                // add a unique count
                else
                    addCount(fallbacks_,fbString);
            }
        }
    }
    // calculate the number of counts for absolute smoothing
    vector<unsigned> oneCounts(n_,0), twoCounts(n_,0), allCounts(n_,0);
    for(KyteaDoubleMap::const_iterator it = probs_.begin(); it != probs_.end(); it++) {
        unsigned n = it->first.length()-1;
        if(it->second == 1.0)
            oneCounts[n]++;
        else if(it->second == 2.0)
            twoCounts[n]++;
        allCounts[n]++;
    }
    // find the discounts according to standard Heuristics (see Chen and Goodman)
    vector<double> discounts(n_,0);
    for(unsigned i = 0; i < n_; i++) {
        if(oneCounts[i] * twoCounts[i] == 0) {
            cerr << "WARNING: Setting discount["<<i<<"] to 0.5 for lack of training data" << endl;
            discounts[i] = 0.5;
        }
        else
            discounts[i] = (double)oneCounts[i]/(oneCounts[i]+twoCounts[i]*2.0);
    }
    // calculate the scores
    for(KyteaDoubleMap::iterator it = probs_.begin(); it != probs_.end(); it++) {
        unsigned n = it->first.length()-1;
        it->second = log((it->second-discounts[n])/denominators[it->first.substr(0,n)]);
    }
    // calculate the fallbacks
    for(KyteaDoubleMap::iterator it = fallbacks_.begin(); it != fallbacks_.end(); it++)
        it->second = log((it->second*discounts[it->first.length()])/denominators[it->first]);
}

double KyteaLM::scoreSingle(const KyteaString & val, int pos) {
    KyteaString ngram(n_);
    for(unsigned i = 0; i < n_; i++) ngram[i] = 0;
    int npos = n_;
    if((int)val.length() == pos) {
        npos--;
        pos--;
    }
    while(--npos >= 0 && pos >= 0)
        ngram[npos] = val[pos--];
    double prob = 0;
    for(npos = 0; npos < (int)n_; npos++) {
        KyteaDoubleMap::const_iterator it = probs_.find(ngram.substr(npos));
        if(it != probs_.end()) {
            prob += it->second;
            return prob;
        } else {
            it = fallbacks_.find(ngram.substr(npos, n_-npos-1));
            if(it != fallbacks_.end())
                prob += it->second;
        }
    }
    return prob + log(1.0/vocabSize_);
}

// score a string with the language model (log probability)
double KyteaLM::score(const KyteaString& val) const {
    unsigned j, len;
    double prob = 0;
    KyteaString testString(val.length()+n_);
    for(j = 0; j < n_-1; j++)
        testString[j] = 0;
    testString[testString.length()-1] = 0;
    testString.splice(val,n_-1);
    for(j = n_; j < testString.length(); j++) {
        for(len = n_; len > 0; len--) {
            KyteaDoubleMap::const_iterator it = const_cast<KyteaDoubleMap*>(&probs_)->find(testString.substr(j-len, len));
            if(it != probs_.end()) {
                prob += it->second;
                break;
            } else {
                it = const_cast<KyteaDoubleMap*>(&fallbacks_)->find(testString.substr(j-len, len-1));
                if(it != fallbacks_.end())
                    prob += it->second;
            }
        }
        if(n_ == 0)
            prob += log(1.0/vocabSize_);
    }
    return prob;
}

void KyteaLM::checkEqual(const KyteaLM & rhs) const {
    if(n_ != rhs.n_)
        THROW_ERROR("KyteaLM n_ don't match: " << n_ << " != " << rhs.n_);
    if(vocabSize_ != rhs.vocabSize_)
        THROW_ERROR("KyteaLM vocabSize_ don't match: " << vocabSize_ << " != " << rhs.vocabSize_);
    checkMapEqual(probs_, rhs.probs_);
    checkMapEqual(fallbacks_, rhs.fallbacks_);
}
