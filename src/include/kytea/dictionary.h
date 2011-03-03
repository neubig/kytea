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

#define DICTIONARY_SAFE

#include "kytea-string.h"
#include "kytea-model.h"
#include <map>
#include <deque>

namespace kytea  {

class PronEntry {
public:
    PronEntry(const KyteaString & str) : word(str), prons(), inDict(0) { }
    virtual ~PronEntry() { }
    
    KyteaString word;
    std::vector<KyteaString> prons;
    unsigned char inDict;

    inline bool isInDict(char test) const {
        return (1 << test) & inDict;
    }
    inline void setInDict(char test) {
        inDict |= (1 << test);
    }
};

class ModelPronEntry : public PronEntry {
public:
    ModelPronEntry(const KyteaString & str) : PronEntry(str), pronMod(0) { }
    ~ModelPronEntry() {
        if(pronMod) 
            delete pronMod;
    }
    
    KyteaModel * pronMod;

};

class ProbPronEntry : public PronEntry {
public:
    ProbPronEntry(const KyteaString & str) : PronEntry(str), probs() { }
    ~ProbPronEntry() { }
    
    double incrementProb(const KyteaString & str) {
        if(probs.size() != prons.size())
            probs.resize(prons.size(), 0.0);
        for(unsigned i = 0; i < prons.size(); i++)
            if(prons[i] == str) 
                return ++probs[i];
        throw std::runtime_error("Attempt to increment a non-existant pronunciation string");
    }

    std::vector< double > probs;

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
        unsigned ret=0,l=0,r=gotos.size(),m;
        KyteaChar check;
        while(r != l) {
            m=(r+l)/2;
            check = gotos[m].first;
            if(input<check) r=m;
            else if(input>check) l=m+1;
            else {
                ret = gotos[m].second;
                break;
            }
        }
        return ret;
    }

};


// a dictionary that uses a FA tree and the Aho-Corasick algorithm for search
//  Aho-Corasick "Efficient String Matching: An Aid to Bibliographic Search"
class Dictionary {

public:

    typedef std::map<KyteaString, PronEntry*> WordMap;
    typedef WordMap::const_iterator wm_const_iterator;

private:

    StringUtil * util_;
    std::vector<DictionaryState*> states_;
    std::vector<PronEntry*> entries_;
    unsigned char numDicts_;

    std::string space(unsigned lev) {
        std::ostringstream oss;
        while(lev-- > 0)
            oss << " ";
        return oss.str();
    }
    void buildGoto(wm_const_iterator start, wm_const_iterator end, unsigned lev, unsigned nid) {
#ifdef DICTIONARY_SAFE
        if(start == end) return;
        if(nid >= states_.size())
            throw std::runtime_error("out of bounds node in buildGoto");
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

    void buildFailures() {
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

public:

    Dictionary(StringUtil * util) : util_(util), numDicts_(0) { };

    void clearData() {
        for(unsigned i = 0; i < states_.size(); i++)
            delete states_[i];
        for(unsigned i = 0; i < entries_.size(); i++)
            delete entries_[i];
        entries_.clear();
        states_.clear();
    }
    ~Dictionary() {
        clearData();
    };

    void buildIndex(const WordMap & input) {
        clearData();
        states_.push_back(new DictionaryState());
        buildGoto(input.begin(), input.end(), 0, 0);
        buildFailures();
    }

    void print() {
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

    const PronEntry * findEntry(KyteaString str) {
        if(str.length() == 0) return 0;
        unsigned state = 0, lev = 0;
        do {
            state = states_[state]->step(str[lev++]);
        } while (state != 0 && lev < str.length());
        if(states_[state]->output.size() == 0) return 0;
        if(!states_[state]->isBranch) return 0;
        return entries_[states_[state]->output[0]];
    }

    unsigned getPronunciationID(KyteaString str, KyteaString pron) {
        const PronEntry * ent = findEntry(str);
        if(ent == 0) return 0;
        for(unsigned i = 0; i < ent->prons.size(); i++)
            if(ent->prons[i] == pron)
                return i+1;
        return 0;
    }
    
    typedef std::vector< std::pair<unsigned,PronEntry*> > MatchResult;
    MatchResult match( const KyteaString & chars ) {
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
                ret.push_back( std::pair<unsigned, PronEntry*>(i, entries_[output[j]]) );
        }
        return ret;
    }

    std::vector<PronEntry*> & getEntries() { return entries_; }
    std::vector<DictionaryState*> & getStates() { return states_; }
    const std::vector<PronEntry*> & getEntries() const { return entries_; }
    const std::vector<DictionaryState*> & getStates() const { return states_; }
    unsigned char getNumDicts() const { return numDicts_; }
    void setNumDicts(unsigned char numDicts) { numDicts_ = numDicts; }

};

}

#endif
