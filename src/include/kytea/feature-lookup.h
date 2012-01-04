#ifndef FEATURE_LOOKUP__
#define FEATURE_LOOKUP__

#include "kytea/dictionary.h"
#include <vector>

namespace kytea {

class FeatureLookup {
protected:
    Dictionary<FeatVec> *charDict_, *typeDict_;
    FeatVec *dictVector_, *biases_;
public:
    FeatureLookup() : charDict_(NULL), typeDict_(NULL), dictVector_(NULL), biases_(NULL) { }
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
    const FeatVal getBias(int id) const {
        return (*biases_)[id];
    }
    const std::vector<FeatVal> * getBiases() const {
        return biases_;
    }

    void addNgramScores(const Dictionary<FeatVec> * dict, 
                        const KyteaString & str,
                        int window,
                        std::vector<FeatSum> & score);

    void addDictionaryScores(
        const Dictionary<ModelTagEntry>::MatchResult & matches,
        int numDicts, int max, std::vector<FeatSum> & score);

    void addTagNgrams(const KyteaString & chars, 
                      const Dictionary<FeatVec> * dict, 
                      std::vector<FeatSum> & scores,
                      int window, int startChar, int endChar);

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
    void setBias(FeatVal bias, int id) {
        if(biases_ == NULL)
            biases_ = new FeatVec(id+1, 0);
        else if((int)biases_->size() <= id)
            biases_->resize(id+1, 0);
        (*biases_)[id] = bias;
    }
    void setBiases(FeatVec * biases) {
        biases_ = biases;
    }


};

}

#endif
