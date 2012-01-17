#ifndef FEATURE_LOOKUP__
#define FEATURE_LOOKUP__

#include "kytea/dictionary.h"
#include <vector>

namespace kytea {

class FeatureLookup {
protected:
    Dictionary<FeatVec> *charDict_, *typeDict_, *selfDict_;
    FeatVec *dictVector_, *biases_, *tagDictVector_, *tagUnkVector_;
    int numTags_;
public:
    FeatureLookup() : charDict_(NULL), typeDict_(NULL), selfDict_(NULL), dictVector_(NULL), biases_(NULL), tagDictVector_(NULL), tagUnkVector_(NULL), numTags_(0) { }
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
        if((selfDict_ == NULL) != (rhs.selfDict_ == NULL)) {
            THROW_ERROR("only one selfDict_ is NULL");
        } else if(selfDict_ != NULL) {
            selfDict_->checkEqual(*rhs.selfDict_);
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
        if((tagDictVector_ == NULL || tagDictVector_->size() == 0) != (rhs.tagDictVector_ == NULL || tagDictVector_->size() == 0)) {
            THROW_ERROR("only one tagDictVector_ is NULL");
        } else if(tagDictVector_ != NULL && tagDictVector_->size() > 0) {
            checkVecEqual(*tagDictVector_, *rhs.tagDictVector_);
        }
        if((tagUnkVector_ == NULL || tagUnkVector_->size() == 0) != (rhs.tagUnkVector_ == NULL || tagUnkVector_->size() == 0)) {
            THROW_ERROR("only one tagUnkVector_ is NULL");
        } else if(tagUnkVector_ != NULL && tagUnkVector_->size() > 0) {
            checkVecEqual(*tagUnkVector_, *rhs.tagUnkVector_);
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
    const FeatVal getTagUnkFeat(int tag) const {
        return (*tagUnkVector_)[tag];
    }
    const FeatVal getTagDictFeat(int dict, int tag, int target) const {
        return (*tagDictVector_)[dict*numTags_*numTags_+tag*numTags_+target];
    }
    const std::vector<FeatVal> * getTagDictVector() const {
        return tagDictVector_;
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

    void addSelfWeights(const KyteaString & chars, 
                        std::vector<FeatSum> & scores,
                        int isType);

    void addTagDictWeights(const std::vector<std::pair<int,int> > & exists, 
                           std::vector<FeatSum> & scores);

    // Setters, these will all take control of the features they are passed
    //  (without making a copy)
    void setCharDict(Dictionary<FeatVec> * charDict) {
        charDict_ = charDict;
    }
    void setTypeDict(Dictionary<FeatVec> * typeDict) {
        typeDict_ = typeDict;
    }
    void setSelfDict(Dictionary<FeatVec> * selfDict) {
        selfDict_ = selfDict;
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
    void setTagDictVector(FeatVec * tagDictVector, int numTags) {
        tagDictVector_ = tagDictVector;
        numTags_ = numTags;
    }
    void setTagUnkVector(FeatVec * tagUnkVector) {
        tagUnkVector_ = tagUnkVector;
    }


};

}

#endif
