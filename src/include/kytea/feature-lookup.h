#ifndef FEATURE_LOOKUP__
#define FEATURE_LOOKUP__

#include <vector>
#include <cstddef>
#include <kytea/feature-vector.h>
#include <kytea/dictionary.h>

namespace kytea {

class KyteaString;
class ModelTagEntry;

class FeatureLookup {
protected:
    Dictionary<FeatVec> *charDict_, *typeDict_, *selfDict_;
    FeatVec *dictVector_, *biases_, *tagDictVector_, *tagUnkVector_;
public:
    FeatureLookup() : charDict_(NULL), typeDict_(NULL), selfDict_(NULL), dictVector_(NULL), biases_(NULL), tagDictVector_(NULL), tagUnkVector_(NULL) { }
    ~FeatureLookup();

    void checkEqual(const FeatureLookup & rhs) const;

    // Getters
    const Dictionary<FeatVec> * getCharDict() const { return charDict_; }
    const Dictionary<FeatVec> * getTypeDict() const { return typeDict_; }
    const Dictionary<FeatVec> * getSelfDict() const { return selfDict_; }
    const FeatVec * getDictVector() const { return dictVector_; }
    const FeatVal getBias(int id) const { return (*biases_)[id]; }
    const std::vector<FeatVal> * getBiases() const { return biases_; }
    const FeatVal getTagUnkFeat(int tag) const { return (*tagUnkVector_)[tag]; }
    // const FeatVal getTagDictFeat(int dict, int tag, int target) const {
    //     return (*tagDictVector_)[dict*numTags_*numTags_+tag*numTags_+target];
    // }
    const std::vector<FeatVal> * getTagDictVector() const { return tagDictVector_; }
    const std::vector<FeatVal> * getTagUnkVector() const { return tagUnkVector_; }

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
    void setCharDict(Dictionary<FeatVec> * charDict) { charDict_ = charDict; }
    void setTypeDict(Dictionary<FeatVec> * typeDict) { typeDict_ = typeDict; }
    void setSelfDict(Dictionary<FeatVec> * selfDict) { selfDict_ = selfDict; }
    void setDictVector(FeatVec * dictVector) { dictVector_ = dictVector; }
    void setBias(FeatVal bias, int id) {
        if(biases_ == NULL)
            biases_ = new FeatVec(id+1, 0);
        else if((int)biases_->size() <= id)
            biases_->resize(id+1, 0);
        (*biases_)[id] = bias;
    }
    void setBiases(FeatVec * biases) { biases_ = biases; }
    void setTagDictVector(FeatVec * tagDictVector) { tagDictVector_ = tagDictVector; }
    void setTagUnkVector(FeatVec * tagUnkVector) { tagUnkVector_ = tagUnkVector; }


};

}

#endif
