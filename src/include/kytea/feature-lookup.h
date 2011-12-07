#ifndef FEATURE_LOOKUP__
#define FEATURE_LOOKUP__

#include "kytea/kytea-model.h"
#include "kytea/dictionary.h"
#include <vector>

namespace kytea {

class FeatureLookup {
protected:
    Dictionary<FeatVec> *charDict_, *typeDict_;
    FeatVec *dictVector_;
    FeatVal bias_;
public:
    FeatureLookup() : charDict_(NULL), typeDict_(NULL), dictVector_(NULL), bias_(0) { }
    ~FeatureLookup();

    // Getters
    const Dictionary<FeatVec> * getCharDict() const {
        return charDict_;
    }
    const Dictionary<FeatVec> * getTypeDict() const {
        return typeDict_;
    }
    const FeatVec * getDictVector() const {
        return dictVector_;
    }
    const FeatVal getBias() const {
        return bias_;
    }

    void addNgramScores(const Dictionary<FeatVec> * dict, 
                        const KyteaString & str,
                        int window,
                        std::vector<FeatSum> & score);

    void addDictionaryScores(
        const Dictionary<ModelTagEntry>::MatchResult & matches,
        int numDicts, int max, std::vector<FeatSum> & score);

    // Setters, these will all take control of the features they are passed
    //  (without making a copy)
    void setCharDict(Dictionary<FeatVec> * charDict) {
        charDict_ = charDict;
    }
    void setTypeDict(Dictionary<FeatVec> * typeDict) {
        typeDict_ = typeDict;
    }
    void setDictVector(FeatVec * dictVector) {
        dictVector_ = dictVector;
    }
    void setBias(FeatVal bias) {
        bias_ = bias;
    }


};

}

#endif
