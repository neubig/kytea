/*
* Copyright 2009-2020, KyTea Development Team
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

#include <algorithm>
#include <set>
#include <cmath>
#include <sstream>
#include <iostream>
#include <kytea/config.h>
#include <kytea/kytea.h>
#include <kytea/dictionary.h>
#include <kytea/corpus-io.h>
#include <kytea/model-io.h>
#include <kytea/feature-io.h>
#include <kytea/dictionary.h>
#include <kytea/string-util.h>
#include <kytea/kytea-util.h>
#include <kytea/kytea-lm.h>
#include <kytea/feature-lookup.h>

using namespace kytea;
using namespace std;

///////////////////////////////////
// Dictionary building functions //
///////////////////////////////////
template <class Entry>
void Kytea::addTag(typename Dictionary<Entry>::WordMap& allWords, const KyteaString & word, int lev, const KyteaString * tag, int dict) {
    typedef typename Dictionary<Entry>::WordMap WordMap;
    typename WordMap::iterator it = allWords.find(word);
    if(it == allWords.end()) {
        Entry * ent = new Entry(word);
        ent->setNumTags(lev+1);
        if(tag) {
            ent->tags[lev].push_back(*tag);
            ent->tagInDicts[lev].push_back(0);
        }
        if(dict >= 0) {
            Entry::setInDict(ent->inDict,dict);
            if(tag)
                Entry::setInDict(ent->tagInDicts[lev][0],dict);
        }
        allWords.insert(typename WordMap::value_type(word,ent));
    }
    else {
        if(tag) {
            unsigned i;
            if((int)it->second->tags.size() <= lev)
                it->second->setNumTags(lev+1);
            vector<KyteaString> & tags = it->second->tags[lev];
            vector<unsigned char> & tagInDicts = it->second->tagInDicts[lev];
            for(i = 0; i < tags.size() && tags[i] != *tag; i++);
            if(i == tags.size()) {
                tags.push_back(*tag);
                tagInDicts.push_back(0);
            }
            if(dict >= 0)
                Entry::setInDict(tagInDicts[i],dict);
        }
        if(dict >= 0) 
            Entry::setInDict(it->second->inDict,dict);
    }
}


template
void Kytea::addTag<ModelTagEntry>(Dictionary<ModelTagEntry>::WordMap& allWords, const KyteaString & word, int lev, const KyteaString * tag, int dict);

template <class Entry>
void Kytea::addTag(typename Dictionary<Entry>::WordMap& allWords, const KyteaString & word, const KyteaTag * tag, int dict) {
    addTag<Entry>(allWords,word,(tag?&tag->first:0),dict);
}

template <class Entry>
void Kytea::scanDictionaries(const vector<string> & dict, typename Dictionary<Entry>::WordMap & wordMap, KyteaConfig * config, StringUtil * util, bool saveIds) {
    // scan the dictionaries
    KyteaString word;
    unsigned char numDicts = 0;
    for(vector<string>::const_iterator it = dict.begin(); it != dict.end(); it++) {
        if(config_->getDebug())
            cerr << "Reading dictionary from " << *it << " ";
        CorpusIO * io = CorpusIO::createIO(it->c_str(), CORP_FORMAT_FULL, *config, false, util);
        io->setNumTags(config_->getNumTags());
        KyteaSentence* next;
        int lines = 0;
        while((next = io->readSentence())) {
            lines++;
            if(next->words.size() != 1) {
                ostringstream buff;
                buff << "Badly formatted dictionary entry (too many or too few words '";
                for(unsigned i = 0; i < next->words.size(); i++) {
                    if(i != 0) buff << " --- ";
                    buff << util->showString(next->words[i].surface);
                }
                buff << "')";
                THROW_ERROR(buff.str());
            }
            word = next->words[0].norm;
            for(int i = 0; i < next->words[0].getNumTags(); i++)
                if(next->words[0].hasTag(i))
                    addTag<Entry>(wordMap, word, i, &next->words[0].getTagSurf(i), (saveIds?numDicts:-1));
            if(next->words[0].getNumTags() == 0)
                addTag<Entry>(wordMap, word, 0, 0, (saveIds?numDicts:-1));
            delete next;
        }
        delete io;
        numDicts++;
        if(config_->getDebug() > 0) {
            if(lines)
                cerr << " done (" << lines  << " entries)" << endl;
            else
                cerr << " WARNING - empty training data specified."  << endl;
        }
    }
}

void Kytea::buildVocabulary() {

    Dictionary<ModelTagEntry>::WordMap & allWords = fio_->getWordMap();

    if(config_->getDebug() > 0)
        cerr << "Scanning dictionaries and corpora for vocabulary" << endl;
    
    // scan the corpora
    vector<string> corpora = config_->getCorpusFiles();
    vector<CorpusFormat> corpForm = config_->getCorpusFormats();
    int maxTag = config_->getNumTags();
    for(unsigned i = 0; i < corpora.size(); i++) {
        if(config_->getDebug() > 0)
            cerr << "Reading corpus from " << corpora[i] << " ";
        CorpusIO * io = CorpusIO::createIO(corpora[i].c_str(), corpForm[i], *config_, false, util_);
        io->setNumTags(config_->getNumTags());
        KyteaSentence* next;
        int lines = 0;
        while((next = io->readSentence())) {
            lines++;
            bool toAdd = false;
            for(unsigned i = 0; i < next->words.size(); i++) {
                if(next->words[i].isCertain) {
                    maxTag = max(next->words[i].getNumTags(),maxTag);
                    for(int j = 0; j < next->words[i].getNumTags(); j++)
                        if(next->words[i].hasTag(j))
                            addTag<ModelTagEntry>(allWords, next->words[i].norm, j, &next->words[i].getTagSurf(j), -1);
                    if(next->words[i].getNumTags() == 0)
                        addTag<ModelTagEntry>(allWords, next->words[i].norm, 0, 0, -1);
                    
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
    config_->setNumTags(maxTag);

    // scan the dictionaries
    scanDictionaries<ModelTagEntry>(config_->getDictionaryFiles(), allWords, config_, util_, true);

    if(sentences_.size() == 0 && fio_->getFeatures().size() == 0)
        THROW_ERROR("There were no sentences in the training data. Check to make sure your training file contains sentences.");

    if(config_->getDebug() > 0)
        cerr << "Building dictionary index ";
    if(allWords.size() == 0)
        THROW_ERROR("FATAL: There were sentences in the training data, but no words were found!");
    if(dict_ != 0) delete dict_;
    dict_ = new Dictionary<ModelTagEntry>(util_);
    dict_->buildIndex(allWords);
    dict_->setNumDicts(max((int)config_->getDictionaryFiles().size(),fio_->getNumDicts()));
    if(config_->getDebug() > 0)
        cerr << "done!" << endl;

}

/////////////////////////////////
// Word segmentation functions //
/////////////////////////////////

unsigned Kytea::wsDictionaryFeatures(const KyteaString & chars, SentenceFeatures & features) {
    // vector<Entry*> & entries = dict_->getEntries();
    // vector<DictionaryState*> & states = dict_->getStates();
    // unsigned currState = 0, nextState;
    ModelTagEntry* myEntry;
    const unsigned len = features.size(), max=config_->getDictionaryN(), dictLen = len*3*max;
    vector<char> on(dict_->getNumDicts()*dictLen, 0);
    unsigned ret = 0, end;
    Dictionary<ModelTagEntry>::MatchResult matches = dict_->match(chars);
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
            char* myOn = &on[di*dictLen + i*3*max];
            for(unsigned j = 0; j < 3*max; j++) {
                unsigned featId = 3*max*di+j;
                if(myOn[j] && dictFeats_[featId]) {
                    features[i].push_back(dictFeats_[featId]);
                    ret++;
                }
            }
        }
    }
    return ret;
}
unsigned Kytea::wsNgramFeatures(const KyteaString & chars, SentenceFeatures & features, const vector<KyteaString> & prefixes, int n) {
    const int featSize = (int)features.size(), 
            charLength = (int)chars.length(),
            w = (int)prefixes.size()/2;
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
    if(config_->getDoWS() && wsModel_) {
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
    TagTriplet * trip = fio_->getFeatures(util_->mapString("WS"),true);
    if(trip->third)
        wsModel_ = trip->third;
    else 
        trip->third = wsModel_ = new KyteaModel();

    if(config_->getDebug() > 0)
        cerr << "Creating word segmentation features ";
    // create word prefixes
    vector<unsigned> dictFeats;
    bool hasDictionary = (dict_->getNumDicts() > 0 && dict_->getStates().size() > 0);
    preparePrefixes();
    // make the sentence features one by one
    unsigned scount = 0;
    vector< vector<unsigned> > & xs = trip->first;
    vector<int> & ys = trip->second;
    for(Sentences::const_iterator it = sentences_.begin(); it != sentences_.end(); it++) {
        if(++scount % 1000 == 0)
            cerr << ".";
        KyteaSentence * sent = *it;
        SentenceFeatures feats(sent->wsConfs.size());
        unsigned fts = 0;
        if(hasDictionary)
            fts += wsDictionaryFeatures(sent->norm, feats);
        fts += wsNgramFeatures(sent->norm, feats, charPrefixes_, config_->getCharN());
        string str = util_->getTypeString(sent->norm);
        fts += wsNgramFeatures(util_->mapString(str), feats, typePrefixes_, config_->getTypeN());
        for(unsigned i = 0; i < feats.size(); i++) {
            if(abs(sent->wsConfs[i]) > config_->getConfidence()) {
                xs.push_back(feats[i]);
                ys.push_back(sent->wsConfs[i]>1?1:-1);
            }
        }
    }
    if(config_->getDebug() > 0)
        cerr << " done!" << endl << "Building classifier ";

    // train the model
    wsModel_->trainModel(xs,ys,config_->getBias(),config_->getSolverType(),config_->getEpsilon(),config_->getCost());

    if(config_->getDebug() > 0)
        cerr << " done!" << endl;

    fio_->printFeatures(util_->mapString("WS"),util_);

}


//////////////////////////////
// Tag estimation functions //
//////////////////////////////

// chars: the string to use to calculate features
// feat: the vector of feature indices
// prefixes: prefixes to use for features
// model: model to use
// n: window to use
// sc: index of the first character before the word
// ec: index of the first character after the word
unsigned Kytea::tagNgramFeatures(const KyteaString & chars, vector<unsigned> & feat, const vector<KyteaString> & prefixes, KyteaModel * model, int n, int sc, int ec) {
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

unsigned Kytea::tagSelfFeatures(const KyteaString & self, vector<unsigned> & feat, const KyteaString & pref, KyteaModel * model) {
    unsigned thisFeat = model->mapFeat(pref+self), ret = 0;
    if(thisFeat) {
        feat.push_back(thisFeat);
        ret++;
    }
    return ret;
}

void Kytea::trainGlobalTags(int lev) {
    if(dict_ == 0)
        return;
    if(config_->getDebug() > 0)
        cerr << "Creating tagging features (tag "<<lev+1<<") ";

    // prepare prefixes
    bool wsAdd = false;
    if(wsModel_) {
        wsAdd = wsModel_->getAddFeatures();
        wsModel_->setAddFeatures(false);
    }
    preparePrefixes();
    if(wsModel_)
        wsModel_->setAddFeatures(wsAdd);

    ostringstream oss; oss << "T "<<lev<<" G";
    KyteaString featId = util_->mapString(oss.str());
    TagTriplet * trip = fio_->getFeatures(featId,true);
    globalMods_[lev] = (trip->third?trip->third:new KyteaModel());
    trip->third = globalMods_[lev];
    KyteaString kssx = util_->mapString("SX"), ksst = util_->mapString("ST");
    
    // build features
    for(Sentences::const_iterator it = sentences_.begin(); it != sentences_.end(); it++) {
        int startPos = 0, finPos=0;
        KyteaString charStr = (*it)->norm;
        KyteaString typeStr = util_->mapString(util_->getTypeString(charStr));
        for(unsigned j = 0; j < (*it)->words.size(); j++) {
            startPos = finPos;
            KyteaWord & word = (*it)->words[j];
            finPos = startPos+word.norm.length();
            if(!word.getTag(lev) || word.getTagConf(lev) <= config_->getConfidence())
                continue;
            unsigned myTag;
            KyteaString tagSurf = word.getTagSurf(lev);
            for(myTag = 0; myTag < trip->fourth.size() && tagSurf != trip->fourth[myTag]; myTag++);
            if(myTag == trip->fourth.size()) 
                trip->fourth.push_back(tagSurf);
            myTag++;
            vector<unsigned> feat;
            tagNgramFeatures(charStr, feat, charPrefixes_, trip->third, config_->getCharN(), startPos-1, finPos);
            tagNgramFeatures(typeStr, feat, typePrefixes_, trip->third, config_->getTypeN(), startPos-1, finPos);
            tagSelfFeatures(word.norm, feat, kssx, trip->third);
            tagSelfFeatures(util_->mapString(util_->getTypeString(word.norm)), feat, ksst, trip->third);
            tagDictFeatures(word.norm, lev, feat, trip->third);
            trip->first.push_back(feat);
            trip->second.push_back(myTag);
        }
    }
    if(config_->getDebug() > 0)
        cerr << "done!" << endl << "Training global tag classifiers ";


    trip->third->trainModel(trip->first,trip->second,config_->getBias(),config_->getSolverType(),config_->getEpsilon(),config_->getCost()); 

    globalTags_[lev] = trip->fourth;
    if(config_->getDebug() > 0)
        cerr << "done with " << globalTags_[lev].size() << " labels and " << 
                trip->third->getNumFeatures() << " features!" << endl;

    fio_->printFeatures(featId,util_);
}

template <class T>
T max(const vector<int> & vec) {
    T myMax = 0;
    for(unsigned i = 0; i < vec.size(); i++)
        if(myMax < vec[i])
            myMax = vec[i];
    return myMax;
}

void Kytea::trainLocalTags(int lev) {
    if(config_->getDebug() > 0)
        cerr << "Creating tagging features (tag "<<lev+1<<") ";
    if(dict_ == 0)
        return;
    // prepare prefixes
    ostringstream oss; oss << "T "<<lev<<" L ";
    KyteaString featId = util_->mapString(oss.str());
    bool wsAdd = false;
    if(wsModel_) {
        wsAdd = wsModel_->getAddFeatures();
        wsModel_->setAddFeatures(false);
    }
    preparePrefixes();
    if(wsModel_)
        wsModel_->setAddFeatures(wsAdd);
    // find words that need to be modeled
    vector<ModelTagEntry*> & entries = dict_->getEntries();
    ModelTagEntry* myEntry = 0;
    for(unsigned i = 0; i < entries.size(); i++) {
        myEntry = entries[i];
        if((int)myEntry->tags.size() > lev && (myEntry->tags[lev].size() > 1 || config_->getWriteFeatures())) {
            TagTriplet * trip = fio_->getFeatures(featId+myEntry->word,true);
            if((int)myEntry->tagMods.size() <= lev)
                myEntry->tagMods.resize(lev+1,0);
            myEntry->tagMods[lev] = (trip->third ? trip->third : new KyteaModel());
            trip->third = myEntry->tagMods[lev];
            trip->fourth = myEntry->tags[lev];
        }
    }
    // build features
    for(Sentences::const_iterator it = sentences_.begin(); it != sentences_.end(); it++) {
        int startPos = 0, finPos=0;
        KyteaString charStr = (*it)->norm;
        KyteaString typeStr = util_->mapString(util_->getTypeString(charStr));
        for(unsigned j = 0; j < (*it)->words.size(); j++) {
            startPos = finPos;
            KyteaWord & word = (*it)->words[j];
            finPos = startPos+word.norm.length();
            if(!word.getTag(lev) || word.getTagConf(lev) <= config_->getConfidence())
                continue;
            TagTriplet * trip = fio_->getFeatures(featId+word.norm,false);
            if(trip) {
                unsigned myTag = dict_->getTagID(word.norm,word.getTagSurf(lev),lev);
                if(myTag != 0) {
                    vector<unsigned> feat;
                    tagNgramFeatures(charStr, feat, charPrefixes_, trip->third, config_->getCharN(), startPos-1, finPos);
                    tagNgramFeatures(typeStr, feat, typePrefixes_, trip->third, config_->getTypeN(), startPos-1, finPos);
                    trip->first.push_back(feat);
                    trip->second.push_back(myTag);
                }
            }
        }
    }
    if(config_->getDebug() > 0)
        cerr << "done!" << endl << "Training local tag classifiers ";
    // calculate classifiers
    for(unsigned i = 0; i < entries.size(); i++) {
        myEntry = entries[i];
        if((int)myEntry->tags.size() > lev && (myEntry->tags[lev].size() > 1 || config_->getWriteFeatures())) {
            TagTriplet * trip = fio_->getFeatures(featId+myEntry->word,false);
            if(!trip) THROW_ERROR("FATAL: Unbuilt model in entry table");
            vector< vector<unsigned> > & xs = trip->first;
            vector<int> & ys = trip->second;
            
            // train the model
            trip->third->trainModel(xs,ys,config_->getBias(),config_->getSolverType(),config_->getEpsilon(),config_->getCost());
            if(trip->third->getNumClasses() == 1) {
                int myLab = trip->third->getLabel(0)-1;
                KyteaString tmpString = myEntry->tags[lev][0]; myEntry->tags[lev][0] = myEntry->tags[lev][myLab]; myEntry->tags[lev][myLab] = tmpString;
                char tmpDict = myEntry->tagInDicts[lev][0]; myEntry->tagInDicts[lev][0] = myEntry->tagInDicts[lev][myLab]; myEntry->tagInDicts[lev][myLab] = tmpDict;
            }
        }
    }

    // print the features
    fio_->printFeatures(featId,util_);

    if(config_->getDebug() > 0)
        cerr << "done!" << endl;
}

vector<pair<int,int> > Kytea::getDictionaryMatches(const KyteaString & surf, int lev) {
    vector<pair<int,int> > ret;
    if(!dict_) return ret;
    const ModelTagEntry* ent = dict_->findEntry(surf);
    if(ent == 0 || ent->inDict == 0 || (int)ent->tagInDicts.size() <= lev)
        return ret;
    // For each tag
    const vector<unsigned char> & tid = ent->tagInDicts[lev];
    for(int i = 0; i < (int)tid.size(); i++) {
        // For each dictionary
        for(int j = 0; j < dict_->getNumDicts(); j++)
            if(ModelTagEntry::isInDict(tid[i],j)) 
                ret.push_back(pair<int,int>(j,i));
    }
    return ret;
}

unsigned Kytea::tagDictFeatures(const KyteaString & surf, int lev, vector<unsigned> & myFeats, KyteaModel * model) {
    vector<pair<int,int> > matches = getDictionaryMatches(surf,lev);
    if(matches.size() == 0) {
        unsigned thisFeat = model->mapFeat(util_->mapString("UNK"));
        if(thisFeat) { myFeats.push_back(thisFeat); return 1; }
        return 0;
    }
    int ret = 0;
    for(int i = 0; i < (int)matches.size(); i++) {
        ostringstream oss; oss << "D" << matches[i].first << "T" << matches[i].second;
        unsigned thisFeat = model->mapFeat(util_->mapString(oss.str()));
        if(thisFeat != 0) {
            myFeats.push_back(thisFeat);
            ret++;
        } 
    }
    return ret;
}

void Kytea::trainSanityCheck() {
    if(config_->getCorpusFiles().size() == 0 && config_->getFeatureIn().length() == 0) {
        THROW_ERROR("At least one input corpus must be specified (-part/-full/-prob)");
    } else if(config_->getDictionaryFiles().size() > 8) {
        THROW_ERROR("The maximum number of dictionaries that can be specified is 8.");
    } else if(config_->getModelFile().length() == 0) {
        THROW_ERROR("An output model file must be specified when training (-model)");
    }
    // check to make sure the model can be output to
    ModelIO * modout = ModelIO::createIO(config_->getModelFile().c_str(),config_->getModelFormat(), true, *config_);
    delete modout;
}


///////////////////////////////
// Unknown word Tag functions //
///////////////////////////////
inline void collectCounts(vector<unsigned> & vec, unsigned pos) {
    for(unsigned i = 0; i < pos; i++) {
        if(vec.size() <= i) vec.push_back(1);
        else                 vec[i]++;
    }
}
void Kytea::trainUnk(int lev) {
    
    // 1. load the subword dictionaries
    if(!subwordDict_) {
        Dictionary<ProbTagEntry>::WordMap subwordMap;
        scanDictionaries<ProbTagEntry>(config_->getSubwordDictFiles(), subwordMap, config_, util_, false);
        subwordDict_ = new Dictionary<ProbTagEntry>(util_);
        subwordDict_->buildIndex(subwordMap);
    }

    // 2. align the pronunciation strings, count subword/tag pairs,
    //     create dictionary of pronunciations to count, collect counts
    if(config_->getDebug() > 0)
        cerr << " Aligning pronunciation strings" << endl;
    typedef vector< pair<unsigned,unsigned> > AlignHyp;
    const vector<ModelTagEntry*> & dictEntries = dict_->getEntries();
    Dictionary<ProbTagEntry>::WordMap tagMap;
    vector<KyteaString> tagCorpus;
    for(unsigned w = 0; w < dictEntries.size(); w++) {
        const TagEntry* myDictEntry = dictEntries[w];
        const KyteaString & word = myDictEntry->word;
        const unsigned wordLen = word.length();
        if((int)myDictEntry->tags.size() <= lev) continue;
        for(unsigned p = 0; p < myDictEntry->tags[lev].size(); p++) {
            const KyteaString & tag = myDictEntry->tags[lev][p];
            vector< vector< AlignHyp > > stacks(wordLen+1, vector< AlignHyp >());
            stacks[0].push_back(AlignHyp(1,pair<unsigned,unsigned>(0,0)));
            // find matches in the word
            Dictionary<ProbTagEntry>::MatchResult matches = subwordDict_->match(word);
            for(unsigned i = 0; i < matches.size(); i++) {
                const ProbTagEntry* mySubEntry = matches[i].second;
                const unsigned cend = matches[i].first+1;
                const unsigned cstart = cend-mySubEntry->word.length();
                for(unsigned j = 0; j < stacks[cstart].size(); j++) {
                    const AlignHyp & myHyp = stacks[cstart][j];
                    const unsigned pstart = myHyp[myHyp.size()-1].second;
                    if((int)mySubEntry->tags.size() <= lev) continue;
                    for(unsigned k = 0; k < mySubEntry->tags[lev].size(); k++) {
                        // if the current hypothesis matches the alignment hypothesis
                        const KyteaString & pstr = mySubEntry->tags[lev][k];
                        const unsigned pend = pstart+pstr.length();
                        if(pend <= tag.length() && tag.substr(pstart,pend-pstart) == pstr) {
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
                if(myHyp[myHyp.size()-1].second == tag.length()) {
                    tagCorpus.push_back(tag);
                    for(unsigned j = 1; j < myHyp.size(); j++) {
                        KyteaString subChar = word.substr(myHyp[j-1].first,myHyp[j].first-myHyp[j-1].first);
                        KyteaString subTag = tag.substr(myHyp[j-1].second,myHyp[j].second-myHyp[j-1].second);
                        ProbTagEntry* mySubEntry = subwordDict_->findEntry(subChar);
                        mySubEntry->incrementProb(subTag,lev);
                        addTag<ProbTagEntry>(tagMap,subTag,lev,&subTag,0);
                    }
                    break;
                }
            }
        }
    }
    if(tagMap.size() == 0) {
        cerr << " No words found! Aborting unknown model for level "<<lev<<endl;
        return;
    }
    if(config_->getDebug() > 0)
        cerr << " Building index" << endl;
    Dictionary<ProbTagEntry> tagDict(util_);
    tagDict.buildIndex(tagMap);

    // 3. put a Dirichlet process prior on the translations, and calculate the ideal alpha
    // count how many unique options there are for each pronunciation
    //  and accumulate the numerator counts
    if(config_->getDebug() > 0)
        cerr << " Calculating alpha" << endl;
    TwoCountHash tagCounts;
    const vector<ProbTagEntry*> & subEntries = subwordDict_->getEntries();
    vector<unsigned> numerCounts;
    for(unsigned w = 0; w < subEntries.size(); w++) {
        const ProbTagEntry* mySubEntry = subEntries[w];
        if((int)mySubEntry->tags.size() <= lev) continue;
        for(unsigned p = 0; p < mySubEntry->tags[lev].size(); p++) {
            const KyteaString& tag = mySubEntry->tags[lev][p];
            TwoCountHash::iterator pcit = tagCounts.find(tag);
            if(pcit == tagCounts.end())
                pcit = tagCounts.insert(TwoCountHash::value_type(tag,TwoCountHash::mapped_type(0,0))).first;
            unsigned totalTagCounts = (unsigned)(mySubEntry->probs[lev].size()>p?mySubEntry->probs[lev][p]:0);
            pcit->second.first++; // add the unique count
            pcit->second.second += totalTagCounts; // add the total count
            collectCounts(numerCounts,totalTagCounts);
        }
    }
    // accumulate the denominator counts
    vector< vector<unsigned> > denomCounts;
    for(TwoCountHash::const_iterator it = tagCounts.begin(); it != tagCounts.end(); it++) {
        if(denomCounts.size() < it->second.first) 
            denomCounts.resize(it->second.first,vector<unsigned>());
        collectCounts(denomCounts[it->second.first-1],it->second.second);
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
        alpha += change;
    }
    if(alpha > maxAlpha) {
        alpha = 1;
        if(config_->getDebug() > 0)
            cerr << "WARNING: Alpha maximization exploded, reverting to alpha="<<alpha<<endl;
    }
    

    // 4. count actual tag phrases and prepare the LM corpus
    for(unsigned p = 0; p < tagCorpus.size(); p++) {
        Dictionary<ProbTagEntry>::MatchResult matches = tagDict.match(tagCorpus[p]);
        for(unsigned m = 0; m < matches.size(); m++)
            matches[m].second->incrementProb(matches[m].second->word,lev);
    }

    // 5. calculate the TM probabilities and adjust with the segmentation probabilities
    for(unsigned w = 0; w < subEntries.size(); w++) {
        ProbTagEntry* mySubEntry = subEntries[w];
        if(mySubEntry->probs[lev].size() != mySubEntry->tags[lev].size())
            mySubEntry->probs[lev].resize(mySubEntry->tags[lev].size(),0);
        for(unsigned p = 0; p < mySubEntry->tags[lev].size(); p++) {
            const KyteaString & tag = mySubEntry->tags[lev][p];
            ProbTagEntry* myTagEntry = tagDict.findEntry(tag);
            double origCount = mySubEntry->probs[lev][p];
            pair<unsigned,unsigned> myTagCounts = tagCounts[tag];
            // get the smoothed TM probability
            mySubEntry->probs[lev][p] = (mySubEntry->probs[lev][p]+alpha) / 
                                   (myTagCounts.second+alpha*myTagCounts.first);
            // adjust it with the segmentation probability (if existing)
            if(myTagEntry)
                mySubEntry->probs[lev][p] *= myTagCounts.second/myTagEntry->probs[lev][0];
            else if (origCount != 0.0) 
                THROW_ERROR("FATAL: Numerator found but denominator not in TM calculation");
            mySubEntry->probs[lev][p] = log(mySubEntry->probs[lev][p]);
        }
    }
    
    // 6. make the language model
    if(config_->getDebug() > 0)
        cerr << " Calculating LM" << endl;
    if((int)subwordModels_.size() <= lev) subwordModels_.resize(lev+1,0);
    subwordModels_[lev] = new KyteaLM(config_->getUnkN());
    subwordModels_[lev]->train(tagCorpus);

}

//////////////////
// IO functions //
//////////////////

void Kytea::buildFeatureLookups() {
    // Write out the word segmentation features
    if(wsModel_) {
        wsModel_->buildFeatureLookup(util_, 
                                     config_->getCharWindow(), config_->getTypeWindow(),
                                     dict_->getNumDicts(), config_->getDictionaryN());
    }
    for(int i = 0; i < (int)globalMods_.size(); i++)
        if(globalMods_[i])
            globalMods_[i]->buildFeatureLookup(util_, 
                                               config_->getCharWindow(), config_->getTypeWindow(),
                                               dict_->getNumDicts(), config_->getDictionaryN());
    // Build the entries for the local models
    vector<ModelTagEntry*> & localEntries = dict_->getEntries();
    for(int i = 0; i < (int)localEntries.size(); i++) {
        if(localEntries[i]) {
            for(int j = 0; j < (int)localEntries[i]->tagMods.size(); j++) {
                if(localEntries[i]->tagMods[j]) {
                    localEntries[i]->tagMods[j]->buildFeatureLookup(util_, 
                                                       config_->getCharWindow(), config_->getTypeWindow(),
                                                       dict_->getNumDicts(), config_->getDictionaryN());    
                }
            }
        }
    }

}

void Kytea::writeModel(const char* fileName) {

    if(config_->getDebug() > 0)    
        cerr << "Printing model to " << fileName;
    // Build the feature lookups before printing
    buildFeatureLookups();

    ModelIO * modout = ModelIO::createIO(fileName,config_->getModelFormat(), true, *config_);
    modout->writeConfig(*config_);
    modout->writeModel(wsModel_);
    // write the global models
    for(int i = 0; i < config_->getNumTags(); i++) {
        modout->writeWordList(i >= (int)globalTags_.size()?vector<KyteaString>():globalTags_[i]);
        modout->writeModel(i >= (int)globalMods_.size()?0:globalMods_[i]);
    }
    modout->writeModelDictionary(dict_);
    modout->writeProbDictionary(subwordDict_);
    for(int i = 0; i < config_->getNumTags(); i++)
        modout->writeLM(i>=(int)subwordModels_.size()?0:subwordModels_[i]);

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
    // Write out the word segmentation features
    wsModel_ = modin->readModel();

    // read the global models
    globalMods_.resize(config_->getNumTags(),0);
    globalTags_.resize(config_->getNumTags(), vector<KyteaString>());
    for(int i = 0; i < config_->getNumTags(); i++) {
        globalTags_[i] = modin->readWordList();
        globalMods_[i] = modin->readModel();
    }
    // read the dictionaries
    dict_ = modin->readModelDictionary();
    subwordDict_ = modin->readProbDictionary();
    subwordModels_.resize(config_->getNumTags(),0);
    for(int i = 0; i < config_->getNumTags(); i++)
        subwordModels_[i] = modin->readLM();

    delete modin;
    
    // prepare the prefixes in advance for faster analysis
    preparePrefixes();

    if(config_->getDebug() > 0)    
        cerr << " done!" << endl;
}


////////////////////////
// Analysis functions //
////////////////////////

void Kytea::calculateWS(KyteaSentence & sent) {
    if(!wsModel_)
        THROW_ERROR("This model cannot be used for word segmentation.");
    
    // Skip empty sentences
    if(sent.norm.length() == 0)
        return;

    // get the features for the sentence
    FeatureLookup * featLookup = wsModel_->getFeatureLookup();
    vector<FeatSum> scores(sent.norm.length()-1, featLookup->getBias(0));
    featLookup->addNgramScores(featLookup->getCharDict(), 
                               sent.norm, config_->getCharWindow(), 
                               scores);
    const string & type_str = util_->getTypeString(sent.norm);
    featLookup->addNgramScores(featLookup->getTypeDict(), 
                               util_->mapString(type_str), 
                               config_->getTypeWindow(), scores);
    if(featLookup->getDictVector())
        featLookup->addDictionaryScores(
            dict_->match(sent.norm),
            dict_->getNumDicts(), config_->getDictionaryN(),
            scores);
    
    // If the characters match the hard constraint, OK
    const string & wsc = config_->getWsConstraint();
    if(wsc.size())
        for(unsigned i = 0; i < scores.size(); i++)
            if(type_str[i]==type_str[i+1] && wsc.find(type_str[i]) != std::string::npos)
                scores[i] = KyteaModel::isProbabilistic(config_->getSolverType())?0:-100;

    // Update values, but only ones that are not already sure
    for(unsigned i = 0; i < sent.wsConfs.size(); i++)
        if(abs(sent.wsConfs[i]) <= config_->getConfidence())
            sent.wsConfs[i] = scores[i]*wsModel_->getMultiplier();
    sent.refreshWS(config_->getConfidence());
    for(int i = 0; i < (int)sent.words.size(); i++) {
        KyteaWord & word = sent.words[i];
        word.setUnknown(dict_->findEntry(word.norm) == 0);
    }
    if(KyteaModel::isProbabilistic(config_->getSolverType())) {
        for(unsigned i = 0; i < sent.wsConfs.size(); i++)
            sent.wsConfs[i] = 1/(1.0+exp(-abs(sent.wsConfs[i])));
    }
}

// generate candidates with TM scores
bool kyteaTagMore(const KyteaTag a, const KyteaTag b) {
    return a.second > b.second;
}

# define BEAM_SIZE 50
vector< KyteaTag > Kytea::generateTagCandidates(const KyteaString & str, int lev) {
    // cerr << "generateTagCandidates("<<util_->showString(str)<<")"<<endl;
    Dictionary<ProbTagEntry>::MatchResult matches = subwordDict_->match(str);
    vector< vector< KyteaTag > > stack(str.length()+1);
    stack[0].push_back(KyteaTag(KyteaString(),0));
    unsigned end, start, lastEnd = 0;
    for(unsigned i = 0; i < matches.size(); i++) {
        // cerr << " match "<<util_->showString(matches[i].second->word)<<" "<<matches[i].first<<endl;
        ProbTagEntry* entry = matches[i].second;
        end = matches[i].first+1;
        start = end-entry->word.length();
        // trim to the beam size
        if(end != lastEnd && config_->getUnkBeam() > 0 && stack[lastEnd].size() > config_->getUnkBeam()) {
            sort(stack[lastEnd].begin(), stack[lastEnd].end(), kyteaTagMore);
            stack[lastEnd].resize(config_->getUnkBeam());
        }
        lastEnd = end;
        // expand the hypotheses
        for(unsigned j = 0; j < entry->tags[lev].size(); j++) {
            for(unsigned k = 0; k < stack[start].size(); k++) {
                KyteaTag nextPair(
                    stack[start][k].first+entry->tags[lev][j],
                    stack[start][k].second+entry->probs[lev][j]
                );
                // cerr << "  ("<<start<<","<<end<<") "<<util_->showString(entry->word)<<", "<<util_->showString(nextPair.first)<<"/"<<nextPair.second;
                for(unsigned pos = stack[start][k].first.length(); pos < nextPair.first.length(); pos++) {
                    nextPair.second += subwordModels_[lev]->scoreSingle(nextPair.first,pos);
                    // cerr << "-->" << nextPair.second;
                }
                // cerr << endl;
                stack[end].push_back(nextPair);
            }
        }
    }
    vector<KyteaTag> ret = stack[stack.size()-1];
    for(unsigned i = 0; i < ret.size(); i++)
        ret[i].second += subwordModels_[lev]->scoreSingle(ret[i].first,ret[i].first.length());
    return ret;
}
void Kytea::calculateUnknownTag(KyteaWord & word, int lev) {
    // cerr << "calculateUnknownTag("<<util_->showString(word.surf)<<")"<<endl;
    if(lev >= (int)subwordModels_.size() || subwordModels_[lev] == 0) return;
    if(word.norm.length() > 256) {
        cerr << "WARNING: skipping pronunciation estimation for extremely long unknown word of length "
            <<word.norm.length()<<" starting with '"
            <<util_->showString(word.norm.substr(0,20))<<"'"<<endl;
        word.addTag(lev, KyteaTag(util_->mapString("<NULL>"),0));
        return;
    }
    // generate candidates
    if((int)word.tags.size() <= lev) word.tags.resize(lev+1);
    word.tags[lev] = generateTagCandidates(word.norm, lev);
    vector<KyteaTag> & tags = word.tags[lev];
    // get the max
    double maxProb = -1e20, totalProb = 0;
    for(unsigned i = 0; i < tags.size(); i++)
        maxProb = max(maxProb,tags[i].second);
    // convert to prob and get the normalizing constant
    for(unsigned i = 0; i < tags.size(); i++) {
        tags[i].second = exp(tags[i].second-maxProb);
        totalProb += tags[i].second;
    }
    // normalize the values
    for(unsigned i = 0; i < tags.size(); i++)
        tags[i].second /= totalProb;
    sort(tags.begin(), tags.end());
    // trim the number of candidates
    if(config_->getTagMax() != 0 && config_->getTagMax() < tags.size())
        tags.resize(config_->getTagMax());

}
void Kytea::calculateTags(KyteaSentence & sent, int lev) {
    int startPos = 0, finPos=0;
    KyteaString charStr = sent.norm;
    KyteaString typeStr = util_->mapString(util_->getTypeString(charStr));
    KyteaString kssx = util_->mapString("SX"), ksst = util_->mapString("ST");
    const string & defTag = config_->getDefaultTag();
    for(unsigned i = 0; i < sent.words.size(); i++) {
        KyteaWord & word = sent.words[i];
        if((int)word.tags.size() > lev
            && (int)word.tags[lev].size() > 0
            && abs(word.tags[lev][0].second) > config_->getConfidence())
                continue;
        startPos = finPos;
        finPos = startPos+word.norm.length();
        // Find the word in the dictionary and set it to unknown if it is
        ModelTagEntry* ent = dict_->findEntry(word.norm);
        word.setUnknown(ent == 0);
        // choose whether to do local or global estimation
        vector<KyteaString> * tags = 0;
        KyteaModel * tagMod = 0;
        bool useSelf = false;
        if(lev < (int)globalMods_.size() && globalMods_[lev] != 0) {
            tagMod = globalMods_[lev];
            tags = &globalTags_[lev];
            useSelf = true;
        }
        else if(ent != 0 && (int)ent->tags.size() > lev) {
            tagMod = ent->tagMods[lev];
            tags = &(ent->tags[lev]);
        }
        // calculate unknown tags
        if(tags == 0 || tags->size() == 0) {
            if(config_->getDoUnk()) {
                calculateUnknownTag(word,lev);
                if(config_->getDebug() >= 2)
                    cerr << "Tag "<<i+1<<" ("<<util_->showString(sent.words[i].surface)<<"->UNK)"<<endl;
            }
        }
        // calculate known tags
        else {
            vector<unsigned> feat;
            FeatureLookup * look;
            if(tagMod == 0 || (look = tagMod->getFeatureLookup()) == NULL)
                word.setTag(lev, KyteaTag((*tags)[0],(KyteaModel::isProbabilistic(config_->getSolverType())?1:100)));
            else {        
#ifdef KYTEA_SAFE
                if(look == NULL) THROW_ERROR("null lookure lookup during analysis");
#endif
                vector<FeatSum> scores(tagMod->getNumWeights(), 0);
                look->addTagNgrams(charStr, look->getCharDict(), scores, config_->getCharN(), startPos, finPos);
                look->addTagNgrams(typeStr, look->getTypeDict(), scores, config_->getTypeN(), startPos, finPos);
                if(useSelf) {
                    look->addSelfWeights(charStr.substr(startPos,finPos-startPos), scores, 0);
                    look->addSelfWeights(typeStr.substr(startPos,finPos-startPos), scores, 1);
                    look->addTagDictWeights(getDictionaryMatches(charStr.substr(startPos,finPos-startPos), 0), scores);
                }
                for(int j = 0; j < (int)scores.size(); j++) 
                    scores[j] += look->getBias(j);
                if(scores.size() == 1)
                    scores.push_back(KyteaModel::isProbabilistic(config_->getSolverType())?-1*scores[0]:0);
                word.clearTags(lev);
                for(int i = 0; i < (int)scores.size(); i++)
                    word.addTag(lev, KyteaTag((*tags)[i],scores[i]*tagMod->getMultiplier()));
                sort(word.tags[lev].begin(), word.tags[lev].end(), kyteaTagMore);
                // Convert to a proper margin or probability
                if(KyteaModel::isProbabilistic(config_->getSolverType())) {
                    double sum = 0;
                    for(int i = 0; i < (int)word.tags[lev].size(); i++) {
                        word.tags[lev][i].second = exp(word.tags[lev][i].second);
                        sum += word.tags[lev][i].second;
                    }
                    for(int i = 0; i < (int)word.tags[lev].size(); i++) {
                        word.tags[lev][i].second /= sum;
                    }
                } else {
                    double secondBest = word.tags[lev][1].second;
                    for(int i = 0; i < (int)word.tags[lev].size(); i++)
                        word.tags[lev][i].second -= secondBest;
                }
            }
        }
        if(!word.hasTag(lev) && defTag.length())
            word.addTag(lev,KyteaTag(util_->mapString(defTag),0));
        if(config_->getTagMax() > 0)
            word.limitTags(lev,config_->getTagMax());
    }
}

// train the analyzer
void Kytea::trainAll() {
    
    // sanity check
    trainSanityCheck();
    
    // handle the feature files
    if(config_->getFeatureIn().length()) {
        if(config_->getDebug() > 0)
            cerr << "Loading features from "<<config_->getFeatureIn() << "...";
        fio_->load(config_->getFeatureIn(),util_);
        if(config_->getDebug() > 0)
            cerr << " done!" << endl;
    }
    config_->setNumTags(max(config_->getNumTags(),fio_->getNumTags()));
    if(config_->getFeatureOut().length())
        fio_->openOut(config_->getFeatureOut());

    // load the vocabulary, tags
    buildVocabulary();
    fio_->setNumTags(config_->getNumTags());
    fio_->printWordMap(util_);

    // train the word segmenter
    if(config_->getDoWS())
        trainWS();
    
    // train the taggers
    if(config_->getDoTags()) {
        if((int)globalMods_.size() <= config_->getNumTags()) {
            globalMods_.resize(config_->getNumTags(),0);
            globalTags_.resize(config_->getNumTags(), vector<KyteaString>());
        }
        for(int i = 0; i < config_->getNumTags(); i++) {
            if(config_->getGlobal(i))
                trainGlobalTags(i);
            else {
                trainLocalTags(i);
                if(config_->getSubwordDictFiles().size() > 0)
                    trainUnk(i);
            }
        }
    }

    // close the feature output
    fio_->closeOut();

    // write the models out to a file
    writeModel(config_->getModelFile().c_str());

}

// load the models and analyze the input
void Kytea::analyze() {
    
    // on full input, disable word segmentation
    if(config_->getInputFormat() == CORP_FORMAT_FULL ||
       config_->getInputFormat() == CORP_FORMAT_TOK)
        config_->setDoWS(false);

    // sanity check
    std::ostringstream buff;
    if(config_->getModelFile().length() == 0)
        throw std::runtime_error("A model file must be specified to run Kytea (-model)");
    
    // read the models in from the model file
    readModel(config_->getModelFile().c_str());
    if(!config_->getDoWS() && !config_->getDoTags()) {
        buff << "Both word segmentation and tagging are disabled." << std::endl
             << "At least one must be selected to perform processing." << std::endl;
        throw std::runtime_error(buff.str());
    }
    // set the input format
    if(config_->getDoWS()) {
        if(config_->getInputFormat() == CORP_FORMAT_DEFAULT)
           config_->setInputFormat(CORP_FORMAT_RAW);
    } else {
        if(config_->getInputFormat() == CORP_FORMAT_DEFAULT)
             config_->setInputFormat(CORP_FORMAT_TOK);
        else if(config_->getInputFormat() == CORP_FORMAT_RAW) {
            buff << "In order to handle raw corpus input, word segmentation must be turned on." << std::endl
                 << "Either specify -in {full,part,prob}, stop using -nows, or train a new " << std::endl
                 << "model that has word segmentation included." << std::endl;
            throw std::runtime_error(buff.str());
        }
    }
    // sanity checks
    if(config_->getDoWS() && wsModel_ == NULL)
        THROW_ERROR("Word segmentation cannot be performed with this model. A new model must be retrained without the -nows option.");

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
    out->setNumTags(config_->getNumTags());
    for(int i = 0; i < config_->getNumTags(); i++)
        out->setDoTag(i,config_->getDoTag(i));

    KyteaSentence* next;
    while((next = in->readSentence()) != 0) {
        if(config_->getDoWS())
            calculateWS(*next);
        if(config_->getDoTags())
            for(int i = 0; i < config_->getNumTags(); i++)
                if(config_->getDoTag(i))
                    calculateTags(*next, i);
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

void Kytea::checkEqual(const Kytea & rhs) {
    checkPointerEqual(util_, rhs.util_);
    // checkPointerEqual(config_, rhs.config_);
    checkPointerEqual(dict_, rhs.dict_);
    checkPointerEqual(wsModel_, rhs.wsModel_);
    checkPointerEqual(subwordDict_, rhs.subwordDict_);
    checkPointerVecEqual(subwordModels_, rhs.subwordModels_);
    checkPointerVecEqual(globalMods_, rhs.globalMods_);
    checkValueVecEqual(globalTags_, rhs.globalTags_);
    checkValueVecEqual(dictFeats_, rhs.dictFeats_);
}

// Destructor and other misc. small functions
Kytea::~Kytea() {
    if(dict_) delete dict_;
    if(subwordDict_) delete subwordDict_;
    if(wsModel_) delete wsModel_;
    if(config_) delete config_;
    if(fio_) delete fio_;
    for(int i = 0; i < (int)subwordModels_.size(); i++) {
        if(subwordModels_[i] != 0) delete subwordModels_[i];
    }
    for(int i = 0; i < (int)globalMods_.size(); i++)
        if(globalMods_[i] != 0) delete globalMods_[i];
    for(Sentences::iterator it = sentences_.begin(); it != sentences_.end(); it++)
        delete *it;
    
}
void Kytea::init() { 
    util_ = config_->getStringUtil();
    // dict_ = new Dictionary(util_);
    dict_ = NULL;
    wsModel_ = NULL;
    subwordDict_ = NULL;
    fio_ = new FeatureIO;
}

template <class Entry>
void Kytea::setDictionary(Dictionary<Entry> * dict) {
    if(dict_ != 0) delete dict_;
    dict_ = dict;
}
template void Kytea::setDictionary(Dictionary<ModelTagEntry> * dict);
