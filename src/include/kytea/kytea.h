/*
* Copyright 2009-2010, KyTea Development Team
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

#ifndef KYTEA_H__
#define KYTEA_H__

#include "kytea-config.h"
#include "kytea-struct.h"
#include "kytea-model.h"
#include "kytea-lm.h"
#include "dictionary.h"

namespace kytea  {

// a class representing the main analyzer
class Kytea {

private:
    typedef unsigned FeatureId;
    typedef std::list<KyteaSentence*> Sentences;
    typedef std::vector< std::vector< FeatureId > > SentenceFeatures;

    StringUtil* util_;
    KyteaConfig* config_;
    Dictionary * dict_;
    Sentences sentences_;

    KyteaModel* wsModel_;

    Dictionary* subwordDict_;
    std::vector<KyteaLM*> subwordModels_;

    std::vector<KyteaModel*> globalMods_;
    std::vector< std::vector<KyteaString> > globalTags_;

    std::vector<unsigned> dictFeats_;
    std::vector<KyteaString> charPrefixes_, typePrefixes_;

public:

///////////////////////////////////////////////////////////////////
//                         API functions                         //
///////////////////////////////////////////////////////////////////

    // Read a model from the file fileName. Character encoding,
    // settings, and other information will be read automatically.
    void readModel(const char* fileName);

    // Writes a model representing the current instance to the
    //  file fileName. The model will be of the type specified
    //  by the parameters in KyteaConfig
    void writeModel(const char* fileName);

    // Calculate the word segmentation for a sentence
    void calculateWS(KyteaSentence & sent);
    
    // Calculate the tagss for a sentence
    void calculateTags(KyteaSentence & sent, int lev);

    // Calculate the unknown pronunciation for a single unknown word
    void calculateUnknownTag(KyteaWord & str, int lev);

    // Get the string utility class that allows you to map to/from
    //  Kyteas internal string representation (using 
    //  mapString/showString)
    StringUtil* getStringUtil() { return config_->getStringUtil(); }

    // Get the the configuration of this isntance of KyTea
    KyteaConfig* getConfig() { return config_; }

    // These are available for convenience, and require you to set
    //  the appropriate settings in KyteaConfig first
    //  "trainAll" performs full training of Kytea from start to finish
    void trainAll();
    //  "analyze" loads models, and analyzes the full corpus input
    void analyze();


///////////////////////////////////////////////////////////////////
//                     Constructor/Destructor                    //
///////////////////////////////////////////////////////////////////

    void init() { 
        util_ = config_->getStringUtil();
        dict_ = new Dictionary(util_);
        dict_ = 0; wsModel_ = 0; subwordDict_ = 0;
    }

    Kytea() : config_(new KyteaConfig()) { init(); }
    Kytea(KyteaConfig & config) : config_(&config) { init(); }
    
    ~Kytea() {
        if(dict_) delete dict_;
        if(subwordDict_) delete subwordDict_;
        if(wsModel_) delete wsModel_;
        for(int i = 0; i < (int)subwordModels_.size(); i++) {
            if(subwordModels_[i] != 0) delete subwordModels_[i];
        }
        for(Sentences::iterator it = sentences_.begin(); it != sentences_.end(); it++)
            delete *it;
    }

private:

///////////////////////////////////////////////////////////////////
// Private functions used internally during Kytea training, etc. //
///////////////////////////////////////////////////////////////////

    // functions to create dictionaries
    void buildVocabulary();
    
    // a function that checks to make sure that configuration is correct before
    //  training
    void trainSanityCheck();

    // functions for word segmentation
    void trainWS();
    void preparePrefixes();
    unsigned wsDictionaryFeatures(const KyteaString & sent, SentenceFeatures & feat);
    unsigned wsNgramFeatures(const KyteaString & sent, SentenceFeatures & feat, const std::vector<KyteaString> & prefixes, int n);

    // functions for tagging
    void trainLocalTags(int lev);
    void trainGlobalTags(int lev);
    unsigned tagCharFeatures(const KyteaString & chars, std::vector<unsigned> & feat, const std::vector<KyteaString> & prefixes, KyteaModel * model, int n, int sc, int ec);
    unsigned tagSelfFeatures(const KyteaString & self, std::vector<unsigned> & feat, const KyteaString & pref, KyteaModel * model);
    unsigned tagDictFeatures(const KyteaString & surf, int lev, std::vector<unsigned> & myFeats, KyteaModel * model);

    // functions for unknown word PE
    void trainUnk(int lev);

    void analyzeInput();
    
    std::vector<KyteaTag> generateTagCandidates(const KyteaString & str, int lev);

};

}

#endif
