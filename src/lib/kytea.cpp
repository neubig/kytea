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

#include <set>
#include <cmath>
#include <kytea/config.h>
#include <kytea/kytea.h>
#include <kytea/corpus-io.h>
#include <kytea/model-io.h>
#include <kytea/dictionary.h>

using namespace kytea;
using namespace std;

class PETriplet {
public:
    vector< vector<unsigned> > first;
    vector<int> second;
    KyteaModel * third;
};

#ifdef HAVE_TR1_UNORDERED_MAP
#   include <tr1/unordered_map>
    typedef std::tr1::unordered_map<KyteaString, PETriplet, KyteaStringHash> PEHash;
    typedef std::tr1::unordered_map<KyteaString, std::pair<unsigned,unsigned>, KyteaStringHash> TwoCountHash;
#elif HAVE_EXT_HASH_MAP
#   include <ext/hash_map>
    typedef __gnu_cxx::hash_map<KyteaString, PETriplet, KyteaStringHash> PEHash;
    typedef __gnu_cxx::hash_map<KyteaString, std::pair<unsigned,unsigned>, KyteaStringHash> TwoCountHash;
#else
#   include <map>
    typedef map<KyteaString, PETriplet> PEHash;
    typedef map<KyteaString, std::pair<unsigned,unsigned>> TwoCountHash;
#endif

///////////////////////////////////
// Dictionary building functions //
///////////////////////////////////
typedef Dictionary::WordMap WordMap;
template <class Entry>
void addPron(Dictionary::WordMap& allWords, const KyteaString & word, const KyteaString * pron, int dict) {
    WordMap::iterator it = allWords.find(word);
    if(it == allWords.end()) {
        Entry * ent = new Entry(word);
        if(pron)
            ent->prons.push_back(*pron);
        if(dict >= 0)
            ent->setInDict(dict);
        allWords.insert(WordMap::value_type(word,ent));
    }
    else {
        if(pron) {
            vector<KyteaString> & prons = it->second->prons;
            if(prons.size() == 0)
                prons.push_back(*pron);
            else if(dict == -1) {
                unsigned i;
                for(i = 0; i < prons.size() && prons[i] != *pron; i++);
                if(i == prons.size())
                    prons.push_back(*pron);
            }
        }
        if(dict >= 0)
            it->second->setInDict(dict);
    }
}
template <class Entry>
void addPron(Dictionary::WordMap& allWords, const KyteaString & word, const KyteaPronunciation * pron, int dict) {
    addPron<Entry>(allWords,word,(pron?&pron->first:0),dict);
}

template <class Entry>
void scanDictionaries(const vector<string> & dict, Dictionary::WordMap & wordMap, KyteaConfig * config, StringUtil * util, bool saveIds = true) {
    // scan the dictionaries
    KyteaString word;
    unsigned char numDicts = 0;
    for(vector<string>::const_iterator it = dict.begin(); it != dict.end(); it++) {
        if(config->getDebug())
            cerr << "Reading dictionary from " << *it << " ";
        CorpusIO * io = CorpusIO::createIO(it->c_str(), CORP_FORMAT_FULL, *config, false, util);
        ifstream dis(it->c_str());
        KyteaSentence* next;
        int lines = 0;
        while((next = io->readSentence())) {
            lines++;
            if(next->words.size() != 1) {
                ostringstream buff;
                buff << "Badly formatted dictionary entry (too many or too few words '";
                for(unsigned i = 0; i < next->words.size(); i++) {
                    if(i != 0) buff << " --- ";
                    buff << util->showString(next->words[i].surf);
                }
                buff << "')";
                throw runtime_error(buff.str());
            }
            word = next->words[0].surf;
            addPron<Entry>(wordMap, word, next->words[0].getPron(), (saveIds?numDicts:-1));
            delete next;
        }
        delete io;
        numDicts++;
        if(config->getDebug() > 0) {
            if(lines)
                cerr << " done (" << lines  << " entries)" << endl;
            else
                cerr << " WARNING - empty training data specified."  << endl;
        }
    }
}

void Kytea::buildVocabulary() {

    Dictionary::WordMap allWords;

    if(config_->getDebug() > 0)
        cerr << "Scanning dictionaries and corpora for vocabulary" << endl;
    
    // scan the corpora
    vector<string> corpora = config_->getCorpusFiles();
    vector<CorpusIO::Format> corpForm = config_->getCorpusFormats();
    for(unsigned i = 0; i < corpora.size(); i++) {
        if(config_->getDebug() > 0)
            cerr << "Reading corpus from " << corpora[i] << " ";
        CorpusIO * io = CorpusIO::createIO(corpora[i].c_str(), corpForm[i], *config_, false, util_);
        KyteaSentence* next;
        int lines = 0;
        while((next = io->readSentence())) {
            lines++;
            bool toAdd = false;
            for(unsigned i = 0; i < next->words.size(); i++) {
                if(next->words[i].isCertain) {
                    addPron<ModelPronEntry>(allWords, next->words[i].surf, next->words[i].getPron(), -1);
                    toAdd = true;
                }
            }
            const unsigned wsSize = next->wsConfs.size();
            for(unsigned i = 0; !toAdd && i < wsSize; i++)
                toAdd = (next->wsConfs[i] != 0);
            if(toAdd)
                sentences_.push_back(next);
            else
                delete next;
        }
        if(config_->getDebug() > 0) {
            if(lines)
                cerr << " done (" << lines  << " lines)" << endl;
            else
                cerr << " WARNING - empty training data specified."  << endl;
        }
        delete io;
    }

    // scan the dictionaries
    scanDictionaries<ModelPronEntry>(config_->getDictionaryFiles(), allWords, config_, util_, true);

    if(sentences_.size() == 0)
        throw runtime_error("There were no sentences in the training data. Check to make sure your training file contains sentences.");

    if(config_->getDebug() > 0)
        cerr << "Building dictionary index ";
    if(allWords.size() == 0)
        throw runtime_error("FATAL: There were sentences in the training data, but no words were found!");
    if(dict_ != 0) delete dict_;
    dict_ = new Dictionary(util_);
    dict_->buildIndex(allWords);
    dict_->setNumDicts(config_->getDictionaryFiles().size());
    if(config_->getDebug() > 0)
        cerr << "done!" << endl;

}


/////////////////////////////////
// Word segmentation functions //
/////////////////////////////////

unsigned Kytea::wsDictionaryFeatures(const KyteaString & chars, SentenceFeatures & features) {
    // vector<PronEntry*> & entries = dict_->getEntries();
    // vector<DictionaryState*> & states = dict_->getStates();
    // unsigned currState = 0, nextState;
    PronEntry* myEntry;
    const unsigned len = features.size(), max=config_->getDictionaryN(), dictLen = len*3*max;
    char* on = (char*)malloc(dict_->getNumDicts()*dictLen);
    unsigned ret = 0, end;
    memset(on,0,dict_->getNumDicts()*dictLen);
    Dictionary::MatchResult matches = dict_->match(chars);
    for(unsigned i = 0; i < matches.size(); i++) {
        end = matches[i].first;
        myEntry = matches[i].second;
        if(myEntry->inDict == 0)
            continue;
        const unsigned wlen = myEntry->word.length();
        const unsigned lablen = min(wlen,max)-1;
        for(unsigned di = 0; ((1 << di) & ~1) <= myEntry->inDict; di++) {
            if(myEntry->isInDict(di)) {
                const unsigned dictOffset = di*dictLen;
                // left value (position end-wlen, type
                if(end >= wlen)
                    on[dictOffset + (end-wlen)*3*max +/*0*max*/+ lablen] = 1;
                // right value
                if(end != len)
                    on[dictOffset +     end*3*max    +  2*max  + lablen] = 1;
                // middle values
                for(unsigned k = end-wlen+1; k < end; k++)
                    on[dictOffset +     k*3*max    +  1*max  + lablen] = 1;
            }
        }
        
    }
    for(unsigned i = 0; i < len; i++) {
        for(unsigned di = 0; di < dict_->getNumDicts(); di++) {
            char* myOn = on+di*dictLen + i*3*max;
            for(unsigned j = 0; j < 3*max; j++) {
                unsigned featId = 3*max*di+j;
                if(myOn[j] && dictFeats_[featId]) {
                    features[i].push_back(dictFeats_[featId]);
                    ret++;
                }
            }
        }
    }
    free(on);
    return ret;
}
unsigned Kytea::wsNgramFeatures(const KyteaString & chars, SentenceFeatures & features, const vector<KyteaString> & prefixes, int n) {
    const int featSize = (int)features.size(), 
            charLength = (int)chars.length(),
            w = (int)prefixes.size()/2;
    // cerr << "featSize=="<<featSize<<", charLength="<<chars.length()<<", w="<<w<<endl;
    // int rightBound, nextRight;
    unsigned ret = 0, thisFeat;
    for(int i = 0; i < featSize; i++) {
        const int rightBound=min(i+w+1,charLength);
        vector<FeatureId> & myFeats = features[i];
        for(int j = i-w+1; j < rightBound; j++) {
            if(j < 0) continue;
            KyteaString str = prefixes[j-i+w-1];
            const int nextRight = min(j+n, rightBound);
            for(int k = j; k<nextRight; k++) {
                str = str+chars[k];
                thisFeat = wsModel_->mapFeat(str);
                if(thisFeat) {
                    myFeats.push_back(thisFeat);
                    ret++;
                }
            }
        }
    }
    return ret;
}



void Kytea::preparePrefixes() {
    // prepare dictionary prefixes
    if(config_->getDoWS()) {
        const char cs[3] = { 'L', 'I', 'R' };
        dictFeats_.resize(0);
        for(unsigned di = 0; di < dict_->getNumDicts(); di++) {
            for(unsigned i = 0; i < 3; i++) {
                for(unsigned j = 0; j < (unsigned)config_->getDictionaryN(); j++) {
                    ostringstream buff;
                    buff << "D" << di << cs[i] << (j+1);
                    dictFeats_.push_back(wsModel_->mapFeat(util_->mapString(buff.str())));
                }
            }
        }
    }
    // create n-gram feature prefixes
    charPrefixes_.resize(0);
    for(int i = 1; i <= 2*(int)config_->getCharWindow(); i++) {
        ostringstream buff;
        buff << "X" << i-(int)config_->getCharWindow();
        charPrefixes_.push_back(util_->mapString(buff.str()));
    }
    typePrefixes_.resize(0);
    for(int i = 1; i <= 2*(int)config_->getTypeWindow(); i++) {
        ostringstream buff;
        buff << "T" << i-(int)config_->getTypeWindow();
        typePrefixes_.push_back(util_->mapString(buff.str()));
    }
}
void Kytea::trainWS() {
    if(wsModel_)
        delete wsModel_;
    wsModel_ = new KyteaModel();
    if(config_->getDebug() > 0)
        cerr << "Creating word segmentation features ";
    // create word prefixes
    vector<unsigned> dictFeats;
    bool hasDictionary = (dict_->getNumDicts() > 0 && dict_->getStates().size() > 0);
    preparePrefixes();
    // make the sentence features one by one
    unsigned scount = 0;
    vector< vector<unsigned> > xs;
    vector<int> ys;
    for(Sentences::const_iterator it = sentences_.begin(); it != sentences_.end(); it++) {
        if(++scount % 1000 == 0)
            cerr << ".";
        KyteaSentence * sent = *it;
        SentenceFeatures feats(sent->wsConfs.size());
        unsigned fts = 0;
        if(hasDictionary)
            fts += wsDictionaryFeatures(sent->chars, feats);
        fts += wsNgramFeatures(sent->chars, feats, charPrefixes_, config_->getCharN());
        string str = util_->getTypeString(sent->chars);
        fts += wsNgramFeatures(util_->mapString(str), feats, typePrefixes_, config_->getTypeN());
        for(unsigned i = 0; i < feats.size(); i++) {
            if(abs(sent->wsConfs[i]) > config_->getConfidence()) {
                // cerr << "feats["<<i<<"] =";
                // for(int j = 0; j < (int)feats[i].size(); j++)
                //     cerr << " "<<util_->showString(wsModel_->showFeat(feats[i][j]));
                // cerr << endl;
                xs.push_back(feats[i]);
                ys.push_back(sent->wsConfs[i]>1?1:-1);
            }
        }
    }
    if(config_->getDebug() > 0)
        cerr << " done!" << endl << "Building classifier ";
    
    wsModel_->trainModel(xs,ys,config_->getBias(),config_->getSolverType(),config_->getEpsilon(),config_->getCost());

    if(config_->getDebug() > 0)
        cerr << " done!" << endl;

}


////////////////////////////////////////
// Pronunciation estimation functions //
////////////////////////////////////////

unsigned Kytea::peCharFeatures(const KyteaString & chars, vector<unsigned> & feat, const vector<KyteaString> & prefixes, KyteaModel * model, int n, int sc, int ec) {
    int w = (int)prefixes.size()/2;
    vector<KyteaChar> wind(prefixes.size());
    for(int i = w-1; i >= 0; i--)
        wind[w-i-1] = (sc-i<0?0:chars[sc-i]);
    for(int i = 0; i < w; i++)
        wind[w+i] = (ec+i>=(int)chars.length()?0:chars[ec+i]);
    unsigned ret = 0, thisFeat = 0;
    for(unsigned i = 0; i < wind.size(); i++) {
        if(wind[i] == 0) continue; 
        KyteaString str = prefixes[i];
        for(int k = 0; k < n && i+k < wind.size() && wind[i+k] != 0; k++) {
            str = str+wind[i+k];
            thisFeat = model->mapFeat(str);
            if(thisFeat) {
                feat.push_back(thisFeat);
                ret++;
            }
        }
    }

    return ret;
}

void Kytea::trainPE() {
    if(config_->getDebug() > 0)
        cerr << "Creating pronunciation estimation features ";
    if(dict_ == 0)
        return;
    // prepare prefixes
    bool wsAdd = wsModel_->getAddFeatures(); wsModel_->setAddFeatures(false);
    preparePrefixes();
    wsModel_->setAddFeatures(wsAdd);
    // find words that need to be modeled
    vector<PronEntry*> & entries = dict_->getEntries();
    ModelPronEntry* myEntry = 0;
    PEHash feats;
    for(unsigned i = 0; i < entries.size(); i++) {
        myEntry = (ModelPronEntry*)entries[i];
        if(myEntry->prons.size() > 1) {
            vector< vector<unsigned> > xs;
            vector<int> ys;
            feats[myEntry->word].first = xs;
            feats[myEntry->word].second = ys;
            if(myEntry->pronMod)
                delete myEntry->pronMod;
            myEntry->pronMod = new KyteaModel();
            feats[myEntry->word].third = myEntry->pronMod;
        }
    }
    // build features
    for(Sentences::const_iterator it = sentences_.begin(); it != sentences_.end(); it++) {
        int startPos = 0, finPos=0;
        KyteaString charStr = (*it)->chars;
        KyteaString typeStr = util_->mapString(util_->getTypeString(charStr));
        for(unsigned j = 0; j < (*it)->words.size(); j++) {
            startPos = finPos;
            KyteaWord & word = (*it)->words[j];
            finPos = startPos+word.surf.length();
            if(!word.getPron() || word.getPronConf() <= config_->getConfidence())
                continue;
            PEHash::iterator peit = feats.find(word.surf);
            if(peit != feats.end()) {
                unsigned myPron = dict_->getPronunciationID(word.surf,word.getPronSurf());
                if(myPron != 0) {
                    vector<unsigned> feat;
                    peCharFeatures(charStr, feat, charPrefixes_, peit->second.third, config_->getCharN(), startPos-1, finPos);
                    peCharFeatures(typeStr, feat, typePrefixes_, peit->second.third, config_->getTypeN(), startPos-1, finPos);
                    peit->second.first.push_back(feat);
                    peit->second.second.push_back(myPron);
                }
            }
        }
    }
    if(config_->getDebug() > 0)
        cerr << "done!" << endl << "Training PE classifiers ";
    // calculate classifiers
    for(PEHash::iterator peit = feats.begin(); peit != feats.end(); peit++) {
        vector< vector<unsigned> > & xs = peit->second.first;
        vector<int> & ys = peit->second.second;
        peit->second.third->trainModel(xs,ys,config_->getBias(),config_->getSolverType(),config_->getEpsilon(),config_->getCost());
    }
    if(config_->getDebug() > 0)
        cerr << "done!" << endl;
}

void Kytea::trainSanityCheck() {
    if(config_->getCorpusFiles().size() == 0)
        throw runtime_error("At least one input corpus must be specified (-part/-full/-prob)");
    else if(config_->getDictionaryFiles().size() > 8)
        throw runtime_error("The maximum number of dictionaries that can be specified is 8.");
    else if(config_->getModelFile().length() == 0)
        throw runtime_error("An output model file must be specified when training (-model)");
    else if(!config_->getDoWS() && !config_->getDoPE())
        throw runtime_error("Both WS and PE are disabled. I can't build a model that estimates nothing!");
    else if(config_->getDoUnk() && !config_->getDoPE())
        throw runtime_error("PE is disabled, but unknown word PE is enabled!");
    if(config_->getDoUnk() && config_->getSubwordDictFiles().size() == 0)
        config_->setDoUnk(false);
    // check to make sure the model can be output to
    ModelIO * modout = ModelIO::createIO(config_->getModelFile().c_str(),config_->getModelFormat(), true, *config_);
    delete modout;
}


///////////////////////////////
// Unknown word PE functions //
///////////////////////////////
inline void collectCounts(vector<unsigned> & vec, unsigned pos) {
    for(unsigned i = 0; i < pos; i++) {
        if(vec.size() <= i) vec.push_back(1);
        else                 vec[i]++;
    }
}
void Kytea::trainUnk() {
    
    // 1. load the subword dictionaries
    WordMap subwordMap;
    scanDictionaries<ProbPronEntry>(config_->getSubwordDictFiles(), subwordMap, config_, util_, false);
    if(subwordDict_) delete subwordDict_;
    subwordDict_ = new Dictionary(util_);
    subwordDict_->buildIndex(subwordMap);

    // 2. align the pronunciation strings, count subword/pron pairs,
    //     create dictionary of pronunciations to count, collect counts
    cerr << " Aligning pronunciation strings" << endl;
    typedef vector< pair<unsigned,unsigned> > AlignHyp;
    const vector<PronEntry*> & dictEntries = dict_->getEntries();
    WordMap pronMap;
    vector<KyteaString> pronCorpus;
    for(unsigned w = 0; w < dictEntries.size(); w++) {
        const PronEntry* myDictEntry = dictEntries[w];
        const KyteaString & word = myDictEntry->word;
        const unsigned wordLen = word.length();
        for(unsigned p = 0; p < myDictEntry->prons.size(); p++) {
            const KyteaString & pron = myDictEntry->prons[p];
            // stacks[endposition][hypothesis][sequencepos].first = word, .second = pron
            vector< vector< AlignHyp > > stacks(wordLen+1, vector< AlignHyp >());
            stacks[0].push_back(AlignHyp(1,pair<unsigned,unsigned>(0,0)));
            // find matches in the word
            Dictionary::MatchResult matches = subwordDict_->match(word);
            for(unsigned i = 0; i < matches.size(); i++) {
                const PronEntry* mySubEntry = matches[i].second;
                const unsigned cend = matches[i].first+1;
                const unsigned cstart = cend-mySubEntry->word.length();
                for(unsigned j = 0; j < stacks[cstart].size(); j++) {
                    const AlignHyp & myHyp = stacks[cstart][j];
                    const unsigned pstart = myHyp[myHyp.size()-1].second;
                    for(unsigned k = 0; k < mySubEntry->prons.size(); k++) {
                        // if the current hypothesis matches the alignment hypothesis
                        const KyteaString & pstr = mySubEntry->prons[k];
                        const unsigned pend = pstart+pstr.length();
                        if(pend <= pron.length() && pron.substr(pstart,pend-pstart) == pstr) {
                            AlignHyp nextHyp = myHyp;
                            nextHyp.push_back( pair<unsigned,unsigned>(cend,pend) );
                            stacks[cend].push_back(nextHyp);
                        }
                    }
                }
            }
            // count the number of alignments
            for(unsigned i = 0; i < stacks[wordLen].size(); i++) {
                const AlignHyp & myHyp = stacks[wordLen][i];
                if(myHyp[myHyp.size()-1].second == pron.length()) {
                    pronCorpus.push_back(pron);
                    for(unsigned j = 1; j < myHyp.size(); j++) {
                        KyteaString subChar = word.substr(myHyp[j-1].first,myHyp[j].first-myHyp[j-1].first);
                        KyteaString subPron = pron.substr(myHyp[j-1].second,myHyp[j].second-myHyp[j-1].second);
                        ProbPronEntry* mySubEntry = (ProbPronEntry*)subwordDict_->findEntry(subChar);
                        mySubEntry->incrementProb(subPron);
                        addPron<ProbPronEntry>(pronMap,subPron,&subPron,0);
                    }
                    break;
                }
            }
        }
    }
    cerr << " Building index" << endl;
    Dictionary pronDict(util_);
    pronDict.buildIndex(pronMap);

    // 3. put a Dirichlet process prior on the translations, and calculate the ideal alpha
    // count how many unique options there are for each pronunciation
    //  and accumulate the numerator counts
    cerr << " Calculating alpha" << endl;
    TwoCountHash pronCounts;
    const vector<PronEntry*> & subEntries = subwordDict_->getEntries();
    vector<unsigned> numerCounts;
    for(unsigned w = 0; w < subEntries.size(); w++) {
        const ProbPronEntry* mySubEntry = (ProbPronEntry*)subEntries[w];
        for(unsigned p = 0; p < mySubEntry->prons.size(); p++) {
            const KyteaString& pron = mySubEntry->prons[p];
            TwoCountHash::iterator pcit = pronCounts.find(pron);
            if(pcit == pronCounts.end())
                pcit = pronCounts.insert(TwoCountHash::value_type(pron,TwoCountHash::mapped_type(0,0))).first;
            unsigned totalPronCounts = (unsigned)(mySubEntry->probs.size()>p?mySubEntry->probs[p]:0);
            pcit->second.first++; // add the unique count
            pcit->second.second += totalPronCounts; // add the total count
            collectCounts(numerCounts,totalPronCounts);
        }
    }
    // accumulate the denominator counts
    vector< vector<unsigned> > denomCounts;
    for(TwoCountHash::const_iterator it = pronCounts.begin(); it != pronCounts.end(); it++) {
        if(denomCounts.size() < it->second.first) 
            denomCounts.resize(it->second.first,vector<unsigned>());
        collectCounts(denomCounts[it->second.first-1],it->second.second);
        // cerr << "uniqueProns["<<util_->showString(it->first)<<"]==" << it->second.first << ", totalProns==" << it->second.second << endl;
    }
    // maximize the alpha using Newton's method
    double alpha = 0.0001, maxAlpha = 100, changeCutoff = 0.0000001, change = 1;
    while(abs(change) > changeCutoff && alpha < maxAlpha) {
        double der1 = 0, der2 = 0, lik = 0, den = 0;
        for(unsigned i = 0; i < numerCounts.size(); i++) {
            den = alpha+i;
            der1 += numerCounts[i]/den;
            der2 -= numerCounts[i]/den/den;
            lik += numerCounts[i]*log(den);
        }
        for(unsigned i = 0; i < denomCounts.size(); i++) {
            for(unsigned j = 0; j < denomCounts[i].size(); j++) {
                den = (i+1)*alpha+j;
                der1 -= denomCounts[i][j]*(i+1)/den;
                der2 += denomCounts[i][j]*(i+1)*(i+1)/den/den;
                lik -= denomCounts[i][j]*log(den);
            }
        }
        change = -1*der1/der2;
        // cerr << "der1="<<der1<<", der2="<<der2<<", lik="<<lik<<", change="<<change<<", alpha="<<alpha<<endl;
        alpha += change;
    }
    if(alpha > maxAlpha) {
        alpha = 1;
        cerr << "WARNING: Alpha maximization exploded, reverting to alpha="<<alpha<<endl;
    }
    

    // 4. count actual pron phrases and prepare the LM corpus
    for(unsigned p = 0; p < pronCorpus.size(); p++) {
        Dictionary::MatchResult matches = pronDict.match(pronCorpus[p]);
        for(unsigned m = 0; m < matches.size(); m++) {
            ProbPronEntry* myPronEntry = (ProbPronEntry*)matches[m].second;
            myPronEntry->incrementProb(myPronEntry->word);
        }
    }

    // 5. calculate the TM probabilities and adjust with the segmentation probabilities
    for(unsigned w = 0; w < subEntries.size(); w++) {
        ProbPronEntry* mySubEntry = (ProbPronEntry*)subEntries[w];
        if(mySubEntry->probs.size() != mySubEntry->prons.size())
            mySubEntry->probs.resize(mySubEntry->prons.size(),0);
        for(unsigned p = 0; p < mySubEntry->prons.size(); p++) {
            const KyteaString & pron = mySubEntry->prons[p];
            ProbPronEntry* myPronEntry = (ProbPronEntry*)pronDict.findEntry(pron);
            double origCount = mySubEntry->probs[p];
            pair<unsigned,unsigned> myPronCounts = pronCounts[pron];
            // get the smoothed TM probability
            // cerr << "mySubEntry->[" << util_->showString(mySubEntry->word) << "/" << util_->showString(pron) << "] == (" << mySubEntry->probs[p] << "+" << alpha << ") / ("<<myPronCounts.second<<"+"<<alpha<<"*"<<myPronCounts.first<<")"<<endl;
            mySubEntry->probs[p] = (mySubEntry->probs[p]+alpha) / 
                                   (myPronCounts.second+alpha*myPronCounts.first);
            // adjust it with the segmentation probability (if existing)
            if(myPronEntry)
                mySubEntry->probs[p] *= myPronCounts.second/myPronEntry->probs[0];
            else if (origCount != 0.0) 
                throw runtime_error("FATAL: Numerator found but denominator not in TM calculation");
            mySubEntry->probs[p] = log(mySubEntry->probs[p]);
            // cerr << "mySubEntry->[" << util_->showString(mySubEntry->word) << "/" << util_->showString(pron) << "] == " << mySubEntry->probs[p] << endl;
            
        }
    }
    
    // 6. make the language model
    cerr << " Calculating LM" << endl;
    subwordModel_ = new KyteaLM(config_->getUnkN());
    subwordModel_->train(pronCorpus);

}

//////////////////
// IO functions //
//////////////////

void Kytea::writeModel(const char* fileName) {

    if(config_->getDebug() > 0)    
        cerr << "Printing model to " << fileName;
    
    ModelIO * modout = ModelIO::createIO(fileName,config_->getModelFormat(), true, *config_);

    modout->writeConfig(*config_);
    modout->writeModel(wsModel_);
    modout->writeModelDictionary(dict_);
    modout->writeProbDictionary(subwordDict_);
    modout->writeLM(subwordModel_);

    delete modout;

    if(config_->getDebug() > 0)    
        cerr << " done!" << endl;

}

void Kytea::readModel(const char* fileName) {
    
    if(config_->getDebug() > 0)
        cerr << "Reading model from " << fileName;

    
    ModelIO * modin = ModelIO::createIO(fileName,ModelIO::FORMAT_UNKNOWN, false, *config_);
    util_ = config_->getStringUtil();

    modin->readConfig(*config_);
    wsModel_ = modin->readModel();
    dict_ = modin->readModelDictionary();
    subwordDict_ = modin->readProbDictionary();
    subwordModel_ = modin->readLM();

    delete modin;
    if(!config_->getDoUnk()) {
        delete subwordModel_;
        subwordModel_ = 0;
        delete subwordDict_;
        subwordDict_ = 0;
    }
    
    // prepare the prefixes in advance for faster analysis
    preparePrefixes();

    if(config_->getDebug() > 0)    
        cerr << " done!" << endl;
}


////////////////////////
// Analysis functions //
////////////////////////

void Kytea::calculateWS(KyteaSentence & sent) {
    
    // get the features for the sentence
    SentenceFeatures feats(sent.wsConfs.size());
    wsDictionaryFeatures(sent.chars, feats);
    wsNgramFeatures(sent.chars, feats, charPrefixes_, config_->getCharN());
    wsNgramFeatures(util_->mapString(util_->getTypeString(sent.chars)), feats, typePrefixes_, config_->getTypeN());

    for(unsigned i = 0; i < sent.wsConfs.size(); i++) {
        // // only parse non-confident values
        // if(abs(sent.wsConfs[i]) <= config_->getConfidence()) {
        // }
        pair<int,double> answer = wsModel_->runClassifier(feats[i])[0];
        // print why each decision was made
        if(config_->getDebug() >= 2) {
            cerr << "WB "<<i+1<<" ("<<util_->showString(sent.chars.substr(i,2))<<"): ";
            wsModel_->printClassifier(feats[i],util_);
            cerr << endl;
        }
        sent.wsConfs[i] = answer.first * abs(answer.second);
    }

    sent.refreshWS(config_->getConfidence());

}

// generate candidates with TM scores
bool kyteaPronMore(const KyteaPronunciation a, const KyteaPronunciation b) {
    return a.second > b.second;
}

# define BEAM_SIZE 50
vector< KyteaPronunciation > Kytea::generatePronCandidates(const KyteaString & str) {
    // cerr << "generatePronCandidates("<<util_->showString(str)<<")"<<endl;
    Dictionary::MatchResult matches = subwordDict_->match(str);
    vector< vector< KyteaPronunciation > > stack(str.length()+1);
    stack[0].push_back(KyteaPronunciation(KyteaString(),0));
    unsigned end, start, lastEnd = 0;
    for(unsigned i = 0; i < matches.size(); i++) {
        // cerr << " match "<<util_->showString(matches[i].second->word)<<" "<<matches[i].first<<endl;
        ProbPronEntry* entry = (ProbPronEntry*)matches[i].second;
        end = matches[i].first+1;
        start = end-entry->word.length();
        // trim to the beam size
        if(end != lastEnd && config_->getUnkBeam() > 0 && stack[lastEnd].size() > config_->getUnkBeam()) {
            sort(stack[lastEnd].begin(), stack[lastEnd].end(), kyteaPronMore);
            stack[lastEnd].resize(config_->getUnkBeam());
        }
        lastEnd = end;
        // expand the hypotheses
        for(unsigned j = 0; j < entry->prons.size(); j++) {
            for(unsigned k = 0; k < stack[start].size(); k++) {
                KyteaPronunciation nextPair(
                    stack[start][k].first+entry->prons[j],
                    stack[start][k].second+entry->probs[j]
                );
                // cerr << "  ("<<start<<","<<end<<") "<<util_->showString(entry->word)<<", "<<util_->showString(nextPair.first)<<"/"<<nextPair.second;
                for(unsigned pos = stack[start][k].first.length(); pos < nextPair.first.length(); pos++) {
                    nextPair.second += subwordModel_->scoreSingle(nextPair.first,pos);
                    // cerr << "-->" << nextPair.second;
                }
                // cerr << endl;
                stack[end].push_back(nextPair);
            }
        }
    }
    vector<KyteaPronunciation> ret = stack[stack.size()-1];
    for(unsigned i = 0; i < ret.size(); i++)
        ret[i].second += subwordModel_->scoreSingle(ret[i].first,ret[i].first.length());
    return ret;
}
void Kytea::calculateUnknownPE(KyteaWord & word) {
    // cerr << "calculateUnknownPE("<<util_->showString(word.surf)<<")"<<endl;
    if(subwordModel_ == 0) return;
    if(word.surf.length() > 256) {
        cerr << "WARNING: skipping pronunciation estimation for extremely long unknown word of length "
            <<word.surf.length()<<" starting with '"
            <<util_->showString(word.surf.substr(0,20))<<"'"<<endl;
        word.prons.push_back(KyteaPronunciation(util_->mapString("<NULL>"),0));
        return;
    }
    // generate candidates
    word.prons = generatePronCandidates(word.surf);
    vector<KyteaPronunciation> & prons = word.prons;
    // get the max
    double maxProb = -1e20, totalProb = 0;
    for(unsigned i = 0; i < prons.size(); i++)
        maxProb = max(maxProb,prons[i].second);
    // convert to prob and get the normalizing constant
    for(unsigned i = 0; i < prons.size(); i++) {
        prons[i].second = exp(prons[i].second-maxProb);
        totalProb += prons[i].second;
    }
    // normalize the values
    for(unsigned i = 0; i < prons.size(); i++)
        prons[i].second /= totalProb;
    sort(prons.begin(), prons.end());
    // trim the number of candidates
    if(config_->getUnkCount() != 0 && config_->getUnkCount() < prons.size())
        prons.resize(config_->getUnkCount());

}
void Kytea::calculatePE(KyteaSentence & sent) {
    int startPos = 0, finPos=0;
    KyteaString charStr = sent.chars;
    KyteaString typeStr = util_->mapString(util_->getTypeString(charStr));
    string defTag = config_->getDefaultTag();
    for(unsigned i = 0; i < sent.words.size(); i++) {
        KyteaWord & word = sent.words[i];
        startPos = finPos;
        finPos = startPos+word.surf.length();
        const ModelPronEntry* ent = (ModelPronEntry*)dict_->findEntry(word.surf);
        if(ent == 0 || ent->prons.size() == 0) {
            calculateUnknownPE(word);
            word.setUnknown(true);
            if(config_->getDebug() >= 2)
                cerr << "PE "<<i+1<<" ("<<util_->showString(sent.words[i].surf)<<"->UNK)"<<endl;

        }
        else {
            word.setUnknown(false);
            vector<unsigned> feat;
            if(ent->pronMod == 0)
                word.setPron(KyteaPronunciation(ent->prons[0],(config_->getSolverType() == 0 || config_->getSolverType() == 6?1:100)));

            else {        
                peCharFeatures(charStr, feat, charPrefixes_, ent->pronMod, config_->getCharN(), startPos-1, finPos);
                peCharFeatures(typeStr, feat, typePrefixes_, ent->pronMod, config_->getTypeN(), startPos-1, finPos);
                vector< pair<int,double> > answer = ent->pronMod->runClassifier(feat);
                word.clearProns();
                for(unsigned j = 0; j < answer.size(); j++)
                    word.addPron(KyteaPronunciation(ent->prons[answer[j].first-1],answer[j].second));
            }
            // print feature info
            if(config_->getDebug() >= 2) {
                cerr << "PE "<<i+1<<" ("<<util_->showString(sent.words[i].surf)<<"->";
                for(int i = 0; i < (int)ent->prons.size(); i++) {
                    if(i != 0) cerr << "/";
                    cerr << util_->showString(ent->prons[i]);
                }
                cerr << ")";
                if(ent->pronMod) { cerr << ": "; ent->pronMod->printClassifier(feat,util_); }
                cerr << endl;
            }
        }
        if(!word.hasPron() && defTag.length())
            word.addPron(KyteaPronunciation(util_->mapString(defTag),0));
    }
}

// train the analyzer
void Kytea::trainAll() {
    
    // sanity check
    trainSanityCheck();
    
    // load the vocabulary, pronunciations
    buildVocabulary();

    // train the word segmenter
    if(config_->getDoWS())
        trainWS();
    
    // train the pronunciation estimator
    if(config_->getDoPE()) {
        trainPE();
        if(config_->getDoUnk() && config_->getSubwordDictFiles().size() > 0)
            trainUnk();
    }

    // write the models out to a file
    writeModel(config_->getModelFile().c_str());

}

// load the models and analyze the input
void Kytea::analyze() {
    
    // on full input, disable word segmentation
    if(config_->getInputFormat() == CORP_FORMAT_FULL)
        config_->setDoWS(false);

    // sanity check
    std::ostringstream buff;
    if(config_->getModelFile().length() == 0)
        throw std::runtime_error("A model file must be specified to run Kytea (-model)");
    
    // read the models in from the model file
    readModel(config_->getModelFile().c_str());
    if(!config_->getDoWS() && !config_->getDoPE()) {
        buff << "Both word segmentation and pronunciation estimation are disabled." << std::endl
             << "At least one must be selected to perform processing." << std::endl;
        throw std::runtime_error(buff.str());
    }
    // set the input format
    if(config_->getDoWS()) {
        if(config_->getInputFormat() == CORP_FORMAT_DEFAULT)
           config_->setInputFormat(CORP_FORMAT_RAW);
    } else {
        if(config_->getInputFormat() == CORP_FORMAT_DEFAULT)
             config_->setInputFormat(CORP_FORMAT_FULL);
        else if(config_->getInputFormat() == CORP_FORMAT_RAW) {
            buff << "In order to handle raw corpus input, word segmentation must be turned on." << std::endl
                 << "Either specify -in {full,part,prob}, stop using -nows, or train a new " << std::endl
                 << "model that has word segmentation included." << std::endl;
            throw std::runtime_error(buff.str());
        }
    }
    // sanity checks
    if(config_->getDoWS() && wsModel_ == 0)
        throw runtime_error("Word segmentation cannot be performed with this model. A new model must be retrained without the -nows option.");

    if(config_->getDebug() > 0)    
        cerr << "Analyzing input ";

    CorpusIO *in, *out;
    iostream *inStr = 0, *outStr = 0;
    const vector<string> & args = config_->getArguments();
    if(args.size() > 0) {
        in  = CorpusIO::createIO(args[0].c_str(),config_->getInputFormat(), *config_, false, util_);
    } else {
        inStr = new iostream(cin.rdbuf());
        in  = CorpusIO::createIO(*inStr, config_->getInputFormat(), *config_, false, util_);
    }
    if(args.size() > 1) {
        out  = CorpusIO::createIO(args[1].c_str(),config_->getOutputFormat(), *config_, true, util_);
    } else {
        outStr = new iostream(cout.rdbuf());
        out = CorpusIO::createIO(*outStr, config_->getOutputFormat(), *config_, true, util_);
    }
    out->setUnkTag(config_->getUnkTag());

    KyteaSentence* next;
    while((next = in->readSentence()) != 0) {
        if(config_->getDoWS())
            calculateWS(*next);
        if(config_->getDoPE())
            calculatePE(*next);
        out->writeSentence(next);
        delete next;
    }

    delete in;
    delete out;
    if(inStr) delete inStr;
    if(outStr) delete outStr;

    if(config_->getDebug() > 0)    
        cerr << "done!" << endl;

}
