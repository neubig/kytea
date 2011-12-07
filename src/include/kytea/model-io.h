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

#ifndef MODEL_IO_H__ 
#define MODEL_IO_H__ 

#include "general-io.h"
#include "kytea-model.h"
#include "kytea-lm.h"
#include "kytea-config.h"
#include "feature-lookup.h"
#include "dictionary.h"
#include "config.h"
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <iostream>


#if DISABLE_QUANTIZE
#   define MODEL_IO_VERSION "0.4.0NQ"
#else
#   define MODEL_IO_VERSION "0.4.0"
#endif

namespace kytea {

class ModelIO : public GeneralIO {

public:

    typedef char Format;
    const static Format FORMAT_BINARY = 'B';
    const static Format FORMAT_TEXT = 'T';
    const static Format FORMAT_UNKNOWN = 'U';

    int numTags_;

public:

    ModelIO(StringUtil* util) : GeneralIO(util) { }
    ModelIO(StringUtil* util, const char* file, bool out, bool bin) : GeneralIO(util,file,out,bin) { }
    ModelIO(StringUtil* util, std::iostream & str, bool out, bool bin) : GeneralIO(util,str,out,bin) { }

    virtual ~ModelIO() { }

    static ModelIO* createIO(const char* file, Format form, bool output, KyteaConfig & config);
    static ModelIO* createIO(std::iostream & str, Format form, bool output, KyteaConfig & config);

    virtual void writeConfig(const KyteaConfig & conf) = 0;
    virtual void writeModel(const KyteaModel * mod) = 0;
    virtual void writeWordList(const std::vector<KyteaString> & list) = 0;
    virtual void writeLM(const KyteaLM * mod) = 0;
    virtual void writeFeatureVector(const FeatVec * vec) = 0;
    virtual void writeFeatureValue(const FeatVal val) = 0;

    virtual void readConfig(KyteaConfig & conf) = 0;
    virtual KyteaModel * readModel() = 0;
    virtual std::vector<KyteaString> readWordList() = 0;
    virtual KyteaLM * readLM() = 0;
    virtual FeatVec * readFeatureVector() = 0;
    virtual FeatVal readFeatureValue() = 0;

    // These must be explicitly expanded because templated virtuals are not allowed
    virtual void writeModelDictionary(const Dictionary<ModelTagEntry> * dict) = 0;
    virtual void writeProbDictionary(const Dictionary<ProbTagEntry> * dict) = 0;
    virtual void writeVectorDictionary(const Dictionary<FeatVec > * dict) = 0;
    virtual Dictionary<ModelTagEntry> * readModelDictionary() = 0;
    virtual Dictionary<ProbTagEntry> * readProbDictionary() = 0;
    virtual Dictionary<FeatVec > * readVectorDictionary() = 0;

    void writeFeatLookup(const FeatureLookup * featLookup) {
        writeVectorDictionary(featLookup->getCharDict());
        writeVectorDictionary(featLookup->getTypeDict());
        writeFeatureVector(featLookup->getDictVector());
        writeFeatureValue(featLookup->getBias());
    }
    FeatureLookup * readFeatLookup() {
        FeatureLookup * look = new FeatureLookup;
        look->setCharDict(readVectorDictionary());
        look->setTypeDict(readVectorDictionary());
        look->setDictVector(readFeatureVector());
        look->setBias(readFeatureValue());
        return look;
    }

};

class TextModelIO : public ModelIO {

public:

    TextModelIO(StringUtil* util) : ModelIO(util) { }
    TextModelIO(StringUtil* util, const char* file, bool out) : ModelIO(util,file,out,false) { }
    TextModelIO(StringUtil* util, std::iostream & str, bool out) : ModelIO(util,str,out,false) { }

    // writing functions

    void writeConfig(const KyteaConfig & conf);
    void writeModel(const KyteaModel * mod);
    void writeWordList(const std::vector<KyteaString> & list);
    void writeModelDictionary(const Dictionary<ModelTagEntry> * dict) { writeDictionary(dict); }
    void writeProbDictionary(const Dictionary<ProbTagEntry> * dict) { writeDictionary(dict); }
    void writeVectorDictionary(const Dictionary<FeatVec > * dict) { writeDictionary(dict); }
    void writeLM(const KyteaLM * mod);
    void writeFeatureVector(const FeatVec * vec);
    void writeFeatureValue(const FeatVal val);

    template <class Entry>
    void writeEntry(const Entry * entry);

    template <class Entry>
    void writeDictionary(const Dictionary<Entry> * dict) {
        if(dict == 0) {
            *str_ << "0" << std::endl << "0" << std::endl;
            return;
        }
        // write the states
        *str_ << (unsigned)dict->getNumDicts() << std::endl;
        const std::vector<DictionaryState*> & states = dict->getStates();
        *str_ << states.size() << std::endl;
        if(states.size() == 0)
            return;
        for(unsigned i = 0; i < states.size(); i++) {
            *str_ << states[i]->failure;
            for(unsigned j = 0; j < states[i]->gotos.size(); j++)
                *str_ << " " << util_->showChar(states[i]->gotos[j].first) << " " << states[i]->gotos[j].second;
            *str_ << std::endl;
            for(unsigned j = 0; j < states[i]->output.size(); j++) {
                if(j!=0) *str_ << " ";
                *str_ << states[i]->output[j];
            }
            *str_ << std::endl;
            *str_ << (states[i]->isBranch?'b':'n') << std::endl;
        }
        // write the entries
        const std::vector<Entry*> & entries = dict->getEntries();
        *str_ << entries.size() << std::endl;
        for(unsigned i = 0; i < entries.size(); i++)
            writeEntry((Entry*)entries[i]);
    }

    // create an appropriate parser based on the type
    static CorpusIO* createIO(const char* file, Format form, bool output, StringUtil* util);
    static CorpusIO* createIO(std::iostream & str, Format form, bool output, StringUtil* util);

    void readConfig(KyteaConfig & conf);
    KyteaModel * readModel();
    std::vector<KyteaString> readWordList();
    Dictionary<ModelTagEntry> * readModelDictionary() { return readDictionary<ModelTagEntry>(); }
    Dictionary<ProbTagEntry> * readProbDictionary()  { return readDictionary<ProbTagEntry>(); }
    Dictionary<FeatVec > * readVectorDictionary()  { return readDictionary<FeatVec >(); }
    KyteaLM * readLM();
    FeatVec * readFeatureVector();
    FeatVal readFeatureValue();

    template <class Entry>
    Entry * readEntry();

    template <class Entry>
    Dictionary<Entry> * readDictionary() {
        Dictionary<Entry> * dict = new Dictionary<Entry>(util_);
        std::string line, buff;
        // get the number of dictionaries
        std::getline(*str_, line);
        dict->setNumDicts(util_->parseInt(line.c_str()));
        // get the states
        std::vector<DictionaryState*> & states = dict->getStates();
        getline(*str_, line);
        states.resize(util_->parseInt(line.c_str()));
        if(states.size() == 0) {
            delete dict;
            return 0;
        }
        for(unsigned i = 0; i < states.size(); i++) {
            DictionaryState * state = new DictionaryState();
            getline(*str_, line);
            std::istringstream iss(line);
            iss >> buff;
            state->failure = util_->parseInt(buff.c_str());
            while(iss >> buff) {
                std::pair<KyteaChar,unsigned> p;
                p.first = util_->mapChar(buff.c_str());
                if(!(iss >> buff))
                    THROW_ERROR("Badly formed model (goto character without a destination)");
                p.second = util_->parseInt(buff.c_str());
                state->gotos.push_back(p);
            }
            sort(state->gotos.begin(), state->gotos.end());
            getline(*str_, line);
            std::istringstream iss2(line);
            while(iss2 >> buff)
                state->output.push_back(util_->parseInt(buff.c_str()));
            getline(*str_, line);
            if(line.length() != 1)
                THROW_ERROR("Badly formed model (branch indicator not found)");
            state->isBranch = (line[0] == 'b');
            states[i] = state;
        }
        // get the entries
        std::vector<Entry*> & entries = dict->getEntries();
        getline(*str_, line);
        entries.resize(util_->parseInt(line.c_str()));
        for(unsigned i = 0; i < entries.size(); i++) {
            entries[i] = readEntry<Entry>();
        }
        return dict;
    }

};

class BinaryModelIO : public ModelIO {

public:

    BinaryModelIO(StringUtil* util) : ModelIO(util) { }
    BinaryModelIO(StringUtil* util, const char* file, bool out) : ModelIO(util,file,out,true) { }
    BinaryModelIO(StringUtil* util, std::iostream & str, bool out) : ModelIO(util,str,out,true) { }

    // output functions

    void writeConfig(const KyteaConfig & conf);
    void writeModel(const KyteaModel * mod);
    void writeWordList(const std::vector<KyteaString> & list);
    void writeModelDictionary(const Dictionary<ModelTagEntry> * dict) { writeDictionary(dict); }
    void writeProbDictionary(const Dictionary<ProbTagEntry> * dict) { writeDictionary(dict); }
    void writeVectorDictionary(const Dictionary<FeatVec > * dict) { writeDictionary(dict); }
    void writeLM(const KyteaLM * mod);
    void writeFeatureVector(const FeatVec * vec);
    void writeFeatureValue(const FeatVal val);

    template <class Entry>
    void writeEntry(const Entry * entry);

    template <class Entry>
    void writeDictionary(const Dictionary<Entry> * dict) {
        // write the number of dicts
        if(dict == 0) {
            writeBinary((unsigned char)0);
            writeBinary((uint32_t)0);
            return;
        }
        if(dict->getNumDicts() > 8)
            THROW_ERROR("Only 8 dictionaries may be stored in a binary file.");
        writeBinary(dict->getNumDicts());
        // write the states
        const std::vector<DictionaryState*> & states = dict->getStates();
        writeBinary((uint32_t)states.size());
        for(unsigned i = 0; i < states.size(); i++) {
            const DictionaryState * state = states[i];
            writeBinary((uint32_t)state->failure);
            writeBinary((uint32_t)state->gotos.size());
            for(unsigned j = 0; j < state->gotos.size(); j++) {
                writeBinary((KyteaChar)state->gotos[j].first);
                writeBinary((uint32_t)state->gotos[j].second);
            }
            writeBinary((uint32_t)state->output.size());
            for(unsigned j = 0; j < state->output.size(); j++) 
                writeBinary((uint32_t)state->output[j]);
            writeBinary(state->isBranch);
        }
        // write the entries
        const std::vector<Entry*> & entries = dict->getEntries();
        writeBinary((uint32_t)entries.size());
        for(unsigned i = 0; i < entries.size(); i++)
            writeEntry(entries[i]);

    }


    // input functions
    void readConfig(KyteaConfig & conf);
    KyteaModel * readModel();
    std::vector<KyteaString> readWordList();
    Dictionary<ModelTagEntry> * readModelDictionary() { return readDictionary<ModelTagEntry>(); }
    Dictionary<ProbTagEntry> * readProbDictionary()  { return readDictionary<ProbTagEntry>(); }
    Dictionary<FeatVec > * readVectorDictionary()  { return readDictionary<FeatVec >(); }
    KyteaLM * readLM();
    FeatVec * readFeatureVector();
    FeatVal readFeatureValue();

    template <class Entry>
    Entry * readEntry();

    template <class Entry>
    Dictionary<Entry> * readDictionary() {
        Dictionary<Entry> * dict = new Dictionary<Entry>(util_);
        std::string line, buff;
        // get the number of dictionaries
        unsigned numDicts = readBinary<unsigned char>();
        dict->setNumDicts(numDicts);
        // get the states
        std::vector<DictionaryState*> & states = dict->getStates();
        states.resize(readBinary<uint32_t>());
        if(states.size() == 0) {
            delete dict;
            return 0;
        }
        for(unsigned i = 0; i < states.size(); i++) {
            DictionaryState * state = new DictionaryState();
            state->failure = readBinary<uint32_t>();
            state->gotos.resize(readBinary<uint32_t>());
            for(unsigned j = 0; j < state->gotos.size(); j++) {
                state->gotos[j].first = readBinary<KyteaChar>();
                state->gotos[j].second = readBinary<uint32_t>();
            }
            state->output.resize(readBinary<uint32_t>());
            for(unsigned j = 0; j < state->output.size(); j++) 
                state->output[j] = readBinary<uint32_t>();
            state->isBranch = readBinary<bool>();
            states[i] = state;
        }
        // get the entries
        std::vector<Entry*> & entries = dict->getEntries();
        entries.resize(readBinary<uint32_t>());
        for(unsigned i = 0; i < entries.size(); i++) 
            entries[i] = readEntry<Entry>();
        return dict;
    }

};

}

#endif
