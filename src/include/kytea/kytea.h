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

#include <kytea/kytea-config.h>
#include <kytea/kytea-struct.h>
#include <vector>

namespace kytea  {

class KyteaTest;
class StringUtil;
class KyteaConfig;
template <class T> class Dictionary;
class ModelTagEntry;
class ProbTagEntry;
class KyteaModel;
class KyteaLM;
class FeatureIO;

// a class representing the main analyzer
class Kytea {

private:
    friend class KyteaTest;
    typedef unsigned FeatureId;
    typedef std::vector<KyteaSentence*> Sentences;
    typedef std::vector< std::vector< FeatureId > > SentenceFeatures;

    StringUtil* util_;
    KyteaConfig* config_;
    Dictionary<ModelTagEntry> * dict_;
    Sentences sentences_;

    // Values for the word segmentation models
    KyteaModel* wsModel_;

    Dictionary<ProbTagEntry>* subwordDict_;
    std::vector<KyteaLM*> subwordModels_;

    std::vector<KyteaModel*> globalMods_;
    std::vector< std::vector<KyteaString> > globalTags_;

    std::vector<unsigned> dictFeats_;
    std::vector<KyteaString> charPrefixes_, typePrefixes_;

    FeatureIO* fio_;

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

    void init();

    Kytea() : config_(new KyteaConfig()) { init(); }
    Kytea(KyteaConfig * config) : config_(config) { init(); }
    
    ~Kytea();

    KyteaModel* getWSModel() { return wsModel_; }

    // Set the word segmentation model and take control of it
    void setWSModel(KyteaModel* model) { wsModel_ = model; }

    // Set the dictionary and take control of it
    template <class Entry>
    void setDictionary(Dictionary<Entry> * dict);

///////////////////////////////////////////////////////////////////
// Functions used internally during Kytea training, testing etc. //
///////////////////////////////////////////////////////////////////

public:

    void checkEqual(const Kytea & rhs);

private:

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
    unsigned tagNgramFeatures(const KyteaString & chars, std::vector<unsigned> & feat, const std::vector<KyteaString> & prefixes, KyteaModel * model, int n, int sc, int ec);
    unsigned tagSelfFeatures(const KyteaString & self, std::vector<unsigned> & feat, const KyteaString & pref, KyteaModel * model);
    unsigned tagDictFeatures(const KyteaString & surf, int lev, std::vector<unsigned> & myFeats, KyteaModel * model);

    // Get matches of the dictionary for a single word in the form of
    // { <x_1, y_1>, <x_2, y_2> }
    // where x is the dictionary and y is the tag that exists in the dicitonary
    std::vector<std::pair<int,int> > getDictionaryMatches(const KyteaString & str, int lev);


    template <class Entry>
    void addTag(typename Dictionary<Entry>::WordMap& allWords, const KyteaString & word, int lev, const KyteaString * tag, int dict);
    template <class Entry>
    void addTag(typename Dictionary<Entry>::WordMap& allWords, const KyteaString & word, const KyteaTag * tag, int dict);
    template <class Entry>
    void scanDictionaries(const std::vector<std::string> & dict, typename Dictionary<Entry>::WordMap & wordMap, KyteaConfig * config, StringUtil * util, bool saveIds = true);

    // functions for unknown word PE
    void trainUnk(int lev);
    void buildFeatureLookups();

    void analyzeInput();
    
    std::vector<KyteaTag> generateTagCandidates(const KyteaString & str, int lev);

};

}

#endif
