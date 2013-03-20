#ifndef MODEL_IO_BINARY_H__
#define MODEL_IO_BINARY_H__

#include <kytea/model-io.h>
#include <kytea/dictionary.h>

namespace kytea {

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
    void writeFeatVec(const FeatVec * vec);
    void writeFeatureLookup(const FeatureLookup * featLookup);

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
    FeatVec * readFeatVec();
    FeatureLookup * readFeatureLookup();

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
