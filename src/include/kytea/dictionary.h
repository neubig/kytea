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

#ifndef KYTEA_DICTIONARY_H_
#define KYTEA_DICTIONARY_H_

#include "kytea/kytea-string.h"
#include "kytea/kytea-model.h"
#include <map>
#include <deque>

namespace kytea  {

class KyteaModel;

class TagEntry {
public:
    TagEntry(const KyteaString & str) : word(str), tags(), inDict(0) { }
    virtual ~TagEntry() { }

    KyteaString word;
    std::vector< std::vector<KyteaString> > tags;
    std::vector< std::vector<unsigned char> > tagInDicts;
    unsigned char inDict;
    
    virtual void setNumTags(int i) {
        tags.resize(i);
        tagInDicts.resize(i);
    }
    
    // check if this is in the dictionary
    inline static bool isInDict(unsigned char in, unsigned char test) {
        return (1 << test) & in;
    }
    inline bool isInDict(unsigned char test) const {
        return isInDict(inDict,test);
    }

    // add this to the dictionary
    inline static void setInDict(unsigned char & in, unsigned char test) {
        in |= (1 << test);
    }
    inline void setInDict(char test) {
        setInDict(inDict,test);
    }
};

class ModelTagEntry : public TagEntry {
public:
    ModelTagEntry(const KyteaString & str) : TagEntry(str) { }
    ~ModelTagEntry();

    void setNumTags(int i) {
        TagEntry::setNumTags(i);
        tagMods.resize(i,0);
    }
    
    std::vector<KyteaModel *> tagMods;

};

class ProbTagEntry : public TagEntry {
public:
    ProbTagEntry(const KyteaString & str) : TagEntry(str), probs() { }
    ~ProbTagEntry() { }
    
    double incrementProb(const KyteaString & str, int lev) {
        // std::cerr << "p.size=="<<probs.size()<<", t.size="<<tags.size()<<std::endl;
        if(probs.size() != tags.size())
            probs.resize(tags.size());
        if(probs[lev].size() != tags[lev].size())
            probs[lev].resize(tags[lev].size(), 0.0);
        for(unsigned i = 0; i < tags[lev].size(); i++)
            if(tags[lev][i] == str) 
                return ++probs[lev][i];
        THROW_ERROR("Attempt to increment a non-existent tag string");
    }

    void setNumTags(int i) {
        TagEntry::setNumTags(i);
        probs.resize(i);
    }

    std::vector< std::vector< double > > probs;

};

class DictionaryState {
public:
    DictionaryState() : failure(0), gotos(), output(), isBranch(false) { }
    
    typedef std::vector< std::pair< KyteaChar, unsigned> > Gotos;

    unsigned failure;
    Gotos gotos;
    std::vector< unsigned > output;
    bool isBranch;

    inline unsigned step(KyteaChar input) {
        Gotos::const_iterator l=gotos.begin(), r=gotos.end(), m;
        KyteaChar check;
        while(r != l) {
            m = l+std::distance(l,r)/2;
            check = m->first;
            if(input<check) r=m;
            else if(input>check) l=m+1;
            else return m->second;
        }
        return 0;
    }

};


// a dictionary that uses a FA tree and the Aho-Corasick algorithm for search
//  Aho-Corasick "Efficient String Matching: An Aid to Bibliographic Search"
template <class Entry>
class Dictionary {

public:

    typedef std::map<KyteaString, Entry*> WordMap;
    typedef typename WordMap::const_iterator wm_const_iterator;

    // A result of dictionary matching, containing pairs of the ending point
    // and the entry
    typedef std::vector< std::pair<unsigned,Entry*> > MatchResult;

private:

    StringUtil * util_;
    std::vector<DictionaryState*> states_;
    std::vector<Entry*> entries_;
    unsigned char numDicts_;

    std::string space(unsigned lev) {
        std::ostringstream oss;
        while(lev-- > 0)
            oss << " ";
        return oss.str();
    }

    // Build the goto and failures for the Aho-Corasick method
    void buildGoto(wm_const_iterator start, wm_const_iterator end, unsigned lev, unsigned nid);
    void buildFailures();

public:

    Dictionary(StringUtil * util) : util_(util), numDicts_(0) { };

    void clearData();

    ~Dictionary() {
        clearData();
    };

    void buildIndex(const WordMap & input);
    void print();

    const Entry * findEntry(KyteaString str) const;
    Entry * findEntry(KyteaString str);
    unsigned getTagID(KyteaString str, KyteaString tag, int lev);

    MatchResult match( const KyteaString & chars ) const;

    std::vector<Entry*> & getEntries() { return entries_; }
    std::vector<DictionaryState*> & getStates() { return states_; }
    const std::vector<Entry*> & getEntries() const { return entries_; }
    const std::vector<DictionaryState*> & getStates() const { return states_; }
    unsigned char getNumDicts() const { return numDicts_; }
    void setNumDicts(unsigned char numDicts) { numDicts_ = numDicts; }

    // This is only a light check to make sure the number of states
    // and entries are identical for now, if necessary expand to check
    // the values as well
    void checkEqual(const Dictionary<Entry> & rhs) const {
        if(states_.size() != rhs.states_.size())
            THROW_ERROR("states_.size() != rhs.states_.size() ("<<states_.size()<<" != "<<rhs.states_.size());
        if(entries_.size() != rhs.entries_.size())
            THROW_ERROR("entries_.size() != rhs.entries_.size() ("<<entries_.size()<<" != "<<rhs.entries_.size());
        if(numDicts_ != rhs.numDicts_)
            THROW_ERROR("numDicts_ != rhs.numDicts_ ("<<numDicts_<<" != "<<rhs.numDicts_);
    }


};

template <class Entry>
void Dictionary<Entry>::buildGoto(wm_const_iterator start, wm_const_iterator end, unsigned lev, unsigned nid) {
#ifdef KYTEA_SAFE
    if(start == end) return;
    if(nid >= states_.size())
        THROW_ERROR("Out of bounds node in buildGoto ("<<nid<<" >= "<<states_.size()<<")");
#endif
    wm_const_iterator startCopy = start;
    DictionaryState & node = *states_[nid];
    // add equal strings
    if(startCopy->first.length() == lev) {
        node.output.push_back(entries_.size());
        node.isBranch = true;
        entries_.push_back(startCopy->second);
        startCopy++;
    }
    if(startCopy == end) return;
    // count the number of buckets
    wm_const_iterator binEnd = startCopy, binStart;
    unsigned numBins = 0;
    KyteaChar lastChar = binEnd->first[lev];
    do {
        binEnd++;
        KyteaChar nextChar = (binEnd == end?0:binEnd->first[lev]);
        if(nextChar != lastChar) {
            numBins++;
            lastChar = nextChar;
        }
    } while(binEnd != end);
    node.gotos.reserve(numBins);
    // add bucket strings
    binStart = startCopy, binEnd = startCopy;
    lastChar = binStart->first[lev];
    do {
        binEnd++;
        KyteaChar nextChar = (binEnd == end?0:binEnd->first[lev]);
        if(nextChar != lastChar) {
            unsigned nextNode = states_.size();
            states_.push_back(new DictionaryState());
            node.gotos.push_back(std::pair<KyteaChar,unsigned>(lastChar,nextNode));
            buildGoto(binStart,binEnd,lev+1,nextNode);
            binStart = binEnd;
            lastChar = nextChar;
        }
    } while(binEnd != end);
}

template <class Entry>
void Dictionary<Entry>::buildFailures() {
    if(states_.size() == 0)
        return;
    std::deque<unsigned> sq;
    DictionaryState::Gotos & g0 = states_[0]->gotos;
    for(unsigned i = 0; i < g0.size(); i++)
        sq.push_back(g0[i].second);
    while(sq.size() != 0) {
        unsigned r = sq.front();
        sq.pop_front();
        DictionaryState::Gotos & gr = states_[r]->gotos;
        for(unsigned i = 0; i < gr.size(); i++) {
            KyteaChar a = gr[i].first;
            unsigned s = gr[i].second;
            sq.push_back(s);
            unsigned state = states_[r]->failure;
            unsigned trans = 0;
            while((trans = states_[state]->step(a)) == 0 && (state != 0))
                state = states_[state]->failure;
            states_[s]->failure = trans;
            for(unsigned j = 0; j < states_[trans]->output.size(); j++)
                states_[s]->output.push_back(states_[trans]->output[j]);
        }
    }

}

template <class Entry>
void Dictionary<Entry>::clearData() {
    for(unsigned i = 0; i < states_.size(); i++)
        delete states_[i];
    for(unsigned i = 0; i < entries_.size(); i++)
        delete entries_[i];
    entries_.clear();
    states_.clear();
}

template <class Entry>
void Dictionary<Entry>::buildIndex(const WordMap & input) {
    if(input.size() == 0)
        THROW_ERROR("Cannot build dictionary for no input");
    clearData();
    states_.push_back(new DictionaryState());
    buildGoto(input.begin(), input.end(), 0, 0);
    buildFailures();
}

template <class Entry>
void Dictionary<Entry>::print() {
    for(unsigned i = 0; i < states_.size(); i++) {
        std::cout << "s="<<i<<", f="<<states_[i]->failure<<", o='";
        for(unsigned j = 0; j < states_[i]->output.size(); j++) {
            if(j!=0) std::cout << " ";
            std::cout << util_->showString(entries_[states_[i]->output[j]]->word);
        }
        std::cout << "' g='";
        for(unsigned j = 0; j < states_[i]->gotos.size(); j++) {
            if(j!=0) std::cout << " ";
            std::cout << util_->showChar(states_[i]->gotos[j].first) << "->" << states_[i]->gotos[j].second;
        }
        std::cout << "'" << std::endl;
    }
}

template <class Entry>
Entry * Dictionary<Entry>::findEntry(KyteaString str) {
    if(str.length() == 0) return 0;
    unsigned state = 0, lev = 0;
    do {
#ifdef KYTEA_SAFE
        if(state >= states_.size())
            THROW_ERROR("Accessing state "<<state<<" that is larger than states_ ("<<states_.size()<<")");
        if(states_[state] == 0)
            THROW_ERROR("Accessing null state "<<state);
#endif
        state = states_[state]->step(str[lev++]);
    } while (state != 0 && lev < str.length());
    if(states_[state]->output.size() == 0) return 0;
    if(!states_[state]->isBranch) return 0;
    return entries_[states_[state]->output[0]];
}
template <class Entry>
const Entry * Dictionary<Entry>::findEntry(KyteaString str) const {
    if(str.length() == 0) return 0;
    unsigned state = 0, lev = 0;
    do {
        state = states_[state]->step(str[lev++]);
    } while (state != 0 && lev < str.length());
    if(states_[state]->output.size() == 0) return 0;
    if(!states_[state]->isBranch) return 0;
    return entries_[states_[state]->output[0]];
}

template <class Entry>
unsigned Dictionary<Entry>::getTagID(KyteaString str, KyteaString tag, int lev) {
    const Entry * ent = findEntry(str);
    if(ent == 0) return 0;
    for(unsigned i = 0; i < ent->tags[lev].size(); i++)
        if(ent->tags[lev][i] == tag)
            return i+1;
    return 0;
}

template <class Entry>
typename Dictionary<Entry>::MatchResult Dictionary<Entry>::match( const KyteaString & chars ) const {
    const unsigned len = chars.length();
    unsigned currState = 0, nextState;
    MatchResult ret;
    for(unsigned i = 0; i < len; i++) {
        KyteaChar c = chars[i];
        while((nextState = states_[currState]->step(c)) == 0 && currState != 0)
            currState = states_[currState]->failure;
        currState = nextState;
        std::vector<unsigned> & output = states_[currState]->output;
        for(unsigned j = 0; j < output.size(); j++) 
            ret.push_back( std::pair<unsigned, Entry*>(i, entries_[output[j]]) );
    }
    return ret;
}

}

#endif
