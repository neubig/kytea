/*
* Copyright 2009, KyTea Development Team
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* 
*     http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef KYTEA_MODEL_H__
#define KYTEA_MODEL_H__

#include <iostream>
#include <kytea/feature-vector.h>
#include <kytea/kytea-string.h>
#include <kytea/kytea-struct.h>

#define SIG_CUTOFF 1E-6

namespace kytea {

typedef std::vector<KyteaString> FeatNameVec;

class FeatureLookup;
template <class Entry>
class Dictionary;

class KyteaModel {
public:

    static inline bool isProbabilistic(int solver) {
        return solver == 0 || solver == 6 || solver == 7;
    }

    static int featuresAdded_;

protected:

    KyteaUnsignedMap ids_;
    FeatNameVec names_;
    FeatNameVec oldNames_;
    std::vector<int> labels_;
    std::vector<FeatVal> weights_;
    double multiplier_;
    double bias_;
    int solver_, numW_;
    bool addFeat_;
    FeatureLookup * featLookup_;

public:
    KyteaModel() : multiplier_(1.0f), bias_(1.0f), solver_(1), addFeat_(true), featLookup_(NULL) {
        KyteaString str;
        mapFeat(str);
    }
    ~KyteaModel();

    // Check that two models are equal, and throw an error if they aren't
    // Mainly used for making sure that model IO is working properly
    void checkEqual(const KyteaModel & rhs) const;

    // feature functions
    inline unsigned mapFeat(const KyteaString & str) {
        KyteaUnsignedMap::const_iterator it = ids_.find(str);
        unsigned ret = 0;
        if(it != ids_.end())
            ret = it->second;
        else if(addFeat_) {
            ret = names_.size();
            ids_[str] = ret;
            names_.push_back(str);
        }
        // std::cerr << "mapFeat:"; for(unsigned i=0;i<str.length();i++) std::cerr << " " << str[i]; std::cerr << " --> "<<ret<<"/"<<names_.size()<<std::endl;
        return ret;
    }
    inline KyteaString showFeat(unsigned val) {
#ifdef KYTEA_SAFE
        if(val >= names_.size())
            THROW_ERROR("FATAL: Array index out of bounds in showFeat ("<<val<<" >= "<<names_.size()<<")");
#endif
        return names_[val];
    }

    int getBiasId() { return (bias_?(int)names_.size():-1); }

    void setAddFeatures(bool addFeat) { addFeat_ = addFeat; }
    bool getAddFeatures() { return addFeat_; }

    const FeatNameVec & getNames() const { return names_; }
    const FeatNameVec & getOldNames() const { return oldNames_; }

    std::vector< std::pair<int,double> > runClassifier(const std::vector<unsigned> & feat);
    // std::pair<int,double> runClassifier(const std::vector<unsigned> & feat);
    void printClassifier(const std::vector<unsigned> & feat, StringUtil * util, std::ostream & out = std::cerr);

    void trainModel(const std::vector< std::vector<unsigned> > & xs, std::vector<int> & ys, double bias, int solver, double epsilon, double cost);
    void trimModel();

    inline const KyteaUnsignedMap & getIds() const { return ids_; }
    inline const unsigned getNumFeatures() const { return names_.size()-1; }
    inline const double getBias() const { return bias_; }
    inline const unsigned getNumWeights() const { return numW_; }
    inline const int getSolver() const { return solver_; }
    inline const unsigned getNumClasses() const { return labels_.size(); }
    inline const int getLabel(unsigned idx) const { return labels_[idx]; }
    inline FeatureLookup * getFeatureLookup() const { return featLookup_; }
    inline const FeatVal getWeight(unsigned i, unsigned j) const {
        int id = i*numW_+j;
#ifdef KYTEA_SAFE
        if(id >= (int)weights_.size())
            THROW_ERROR("weight out of bounds: size="<<weights_.size()<<" id="<<id);
#endif
        // std::cerr << "getWeight("<<i<<","<<j<<") == "<<weights_[id]<<std::endl;
        return weights_[id];
    }
    const double getMultiplier() const { return multiplier_; }
    void setBias(double bias) { bias_ = bias; }
    void setLabel(unsigned i, int lab) { labels_[i] = lab; }
    void setSolver(int i) { solver_ = i; }
    void setNumWeights(int i) { numW_ = i; }
    void setFeatureLookup(FeatureLookup * featLookup) { featLookup_ = featLookup; }
    void setNumFeatures(unsigned i);
    void setNumClasses(unsigned i);

    void initializeWeights(unsigned i, unsigned j) { weights_.resize(i*j,0); }
    void setWeight(unsigned i, unsigned j, FeatVal w) { 
        int id = i*numW_+j;
#ifdef KYTEA_SAFE
        if(id >= (int)weights_.size())
            THROW_ERROR("weight out of bounds: size="<<weights_.size()<<" id="<<id);
#endif
        weights_[id] = w;
    }
    void setMultiplier(double m) { multiplier_ = m; }

    void buildFeatureLookup(StringUtil * util, int charw, int typew, int numDicts, int maxLen);
    Dictionary<std::vector<FeatVal> > * 
        makeDictionaryFromPrefixes(const std::vector<KyteaString> & prefs, StringUtil* util, bool adjustPos);
    

};

class TagTriplet {
public:
    std::vector< std::vector<unsigned> > first;
    std::vector<int> second;
    KyteaModel * third;
    std::vector<KyteaString> fourth;

    TagTriplet() : first(), second(), third(0), fourth() { }
};

typedef KyteaStringMap<TagTriplet*> TagHash;

}

#endif
