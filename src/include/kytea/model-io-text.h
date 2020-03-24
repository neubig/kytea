#ifndef MODEL_IO_TEXT_H__
#define MODEL_IO_TEXT_H__

#include <kytea/model-io.h>
#include <algorithm>

namespace kytea {

class CorpusIO;

class TextModelIO : public ModelIO {

public:

    TextModelIO(StringUtil* util) : ModelIO(util) { }
    TextModelIO(StringUtil* util, const char* file, bool out) : ModelIO(util,file,out,false) { }
    TextModelIO(StringUtil* util, std::iostream & str, bool out) : ModelIO(util,str,out,false) { }

    // writing functions

    void writeConfig(const KyteaConfig & conf) override;
    void writeModel(const KyteaModel * mod) override;
    void writeWordList(const std::vector<KyteaString> & list) override;
    void writeModelDictionary(const Dictionary<ModelTagEntry> * dict) override { writeDictionary(dict); }
    void writeProbDictionary(const Dictionary<ProbTagEntry> * dict) override { writeDictionary(dict); }
    void writeVectorDictionary(const Dictionary<FeatVec > * dict) override { writeDictionary(dict); }
    void writeLM(const KyteaLM * mod) override;
    void writeFeatVec(const FeatVec * vec) override;
    void writeFeatureLookup(const FeatureLookup * featLookup) override;

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

    void readConfig(KyteaConfig & conf) override;
    KyteaModel * readModel() override;
    std::vector<KyteaString> readWordList() override;
    Dictionary<ModelTagEntry> * readModelDictionary() override { return readDictionary<ModelTagEntry>(); }
    Dictionary<ProbTagEntry> * readProbDictionary() override { return readDictionary<ProbTagEntry>(); }
    Dictionary<FeatVec > * readVectorDictionary() override { return readDictionary<FeatVec >(); }
    KyteaLM * readLM() override;
    FeatVec * readFeatVec() override;
    FeatureLookup * readFeatureLookup() override;

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

}

#endif
