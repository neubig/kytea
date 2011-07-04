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

#include "kytea-struct.h"
#include "kytea-string.h"
#include "string-util.h"
#include <vector>
#include <iostream>

#define MODEL_SAFE
#define SIG_CUTOFF 1E-6

namespace kytea {

class KyteaModel {
public:

#if DISABLE_QUANTIZE
    typedef double FeatVal;
    typedef double FeatSum;
#else
    typedef short FeatVal;
    typedef int FeatSum;
#endif
    typedef std::vector<KyteaString> FeatVec;

    static inline bool isProbabilistic(int solver) {
        return solver == 0 || solver == 6 || solver == 7;
    }

private:

    KyteaUnsignedMap ids_;
    FeatVec names_;
    FeatVec oldNames_;
    std::vector<int> labels_;
    std::vector<FeatVal> weights_;
    double multiplier_;
    double bias_;
    int solver_, numW_;
    bool addFeat_;

public:
    KyteaModel() : names_(), multiplier_(0.0f), bias_(1.0f), solver_(1), addFeat_(true) {
        KyteaString str;
        mapFeat(str);
    }
    ~KyteaModel() { }

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
#ifdef MODEL_SAFE
        if(val >= names_.size())
            THROW_ERROR("FATAL: Array index out of bounds in showFeat ("<<val<<" >= "<<names_.size()<<")");
#endif
        return names_[val];
    }

    int getBiasId() { return (bias_?(int)names_.size():-1); }

    void setAddFeatures(bool addFeat) { addFeat_ = addFeat; }
    bool getAddFeatures() { return addFeat_; }

    const FeatVec & getNames() const { return names_; }
    const FeatVec & getOldNames() const { return oldNames_; }

    std::vector< std::pair<int,double> > runClassifier(const std::vector<unsigned> & feat);
    // std::pair<int,double> runClassifier(const std::vector<unsigned> & feat);
    void printClassifier(const std::vector<unsigned> & feat, StringUtil * util, std::ostream & out = std::cerr);

    void trainModel(const std::vector< std::vector<unsigned> > & xs, std::vector<int> & ys, double bias, int solver, double epsilon, double cost);
    void trimModel();

    inline const unsigned getNumFeatures() const { return names_.size()-1; }
    inline const double getBias() const { return bias_; }
    inline const unsigned getNumWeights() const { return numW_; }
    inline const int getSolver() const { return solver_; }
    inline const unsigned getNumClasses() const { return labels_.size(); }
    inline const int getLabel(unsigned idx) const { return labels_[idx]; }
    inline const FeatVal getWeight(unsigned i, unsigned j) const { return weights_[i*numW_+j]; }
    inline const double getMultiplier() const { return multiplier_; }

    inline void setBias(double bias) { bias_ = bias; }
    inline void setLabel(unsigned i, int lab) { labels_[i] = lab; }
    inline void setSolver(int i) { solver_ = i; }
    inline void setNumFeatures(unsigned i) {
        if(i != getNumFeatures()) 
            THROW_ERROR("setting the number of features to a different value is not allowed ("<<i<<" != "<<getNumFeatures()<<")");
    }
    void setNumClasses(unsigned i);

    void initializeWeights(unsigned i, unsigned j) { weights_.resize(i*j,0); }
    void setWeight(unsigned i, unsigned j, FeatVal w) { weights_[i*numW_+j] = w; }
    void setMultiplier(double m) { multiplier_ = m; }
    

};

class TagTriplet {
public:
    std::vector< std::vector<unsigned> > first;
    std::vector<int> second;
    KyteaModel * third;
    std::vector<KyteaString> fourth;

    TagTriplet() : first(), second(), third(0), fourth() { }
};

// maps for use with various classes
#ifdef HAVE_TR1_UNORDERED_MAP
#   include <tr1/unordered_map>
    typedef std::tr1::unordered_map<KyteaString, TagTriplet*, KyteaStringHash> TagHash;
#elif HAVE_EXT_HASH_MAP
#   include <ext/hash_map>
    namespace __gnu_cxx {
    template <>
    struct hash<std::string> {
        size_t operator() (const std::string& x) const { return hash<const char*>()(x.c_str()); }
    };
    }
    typedef __gnu_cxx::hash_map<KyteaString, TagTriplet*, KyteaStringHash> TagHash;
#else
#   include <map>
    typedef std::map<KyteaString, TagTriplet*> TagHash;
#endif

}

#endif
