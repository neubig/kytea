#ifndef FEATURE_LOOKUP__
#define FEATURE_LOOKUP__

#include "kytea/dictionary.h"
#include <vector>

namespace kytea {

class FeatureLookup {
protected:
    Dictionary<FeatVec> *charDict_, *typeDict_;
    FeatVec *dictVector_, *biases_;
    KyteaStringMap<FeatVec> *charSelf_, *typeSelf_;
public:
    FeatureLookup() : charDict_(NULL), typeDict_(NULL), dictVector_(NULL), biases_(NULL), charSelf_(NULL), typeSelf_(NULL) { }
    ~FeatureLookup();

    void checkEqual(const FeatureLookup & rhs) const {
        if((charDict_ == NULL) != (rhs.charDict_ == NULL)) {
            THROW_ERROR("only one charDict_ is NULL");
        } else if(charDict_ != NULL) {
            charDict_->checkEqual(*rhs.charDict_);
        }
        if((typeDict_ == NULL) != (rhs.typeDict_ == NULL)) {
            THROW_ERROR("only one typeDict_ is NULL");
        } else if(typeDict_ != NULL) {
            typeDict_->checkEqual(*rhs.typeDict_);
        }
        if((dictVector_ == NULL || dictVector_->size() == 0) != (rhs.dictVector_ == NULL || rhs.dictVector_->size() == 0)) {
            THROW_ERROR("only one dictVector_ is NULL");
        } else if(dictVector_ != NULL && dictVector_->size() > 0) {
            checkVecEqual(*dictVector_, *rhs.dictVector_);
        }
        if((biases_ == NULL || biases_->size() == 0) != (rhs.biases_ == NULL || biases_->size() == 0)) {
            THROW_ERROR("only one biases_ is NULL");
        } else if(biases_ != NULL && biases_->size() > 0) {
            checkVecEqual(*biases_, *rhs.biases_);
        }
        if((charSelf_ == NULL) != (rhs.charSelf_ == NULL)) {
            THROW_ERROR("only one charSelf_ is NULL");
        } else if(charSelf_ != NULL) {
            checkMapEqual(*charSelf_, *rhs.charSelf_);
        }
        if((typeSelf_ == NULL) != (rhs.typeSelf_ == NULL)) {
            THROW_ERROR("only one typeSelf_ is NULL");
        } else if(typeSelf_ != NULL) {
            checkMapEqual(*typeSelf_, *rhs.typeSelf_);
        }
    }

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
