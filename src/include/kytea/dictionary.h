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

#include <kytea/kytea-string.h>
#include <kytea/string-util.h>
// #include <kytea/kytea-model.h>
#include <map>
#include <deque>

namespace kytea  {

class KyteaModel;
class KyteaString;

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

    void setNumTags(int i) override {
        TagEntry::setNumTags(i);
        tagMods.resize(i,0);
    }
    
    std::vector<KyteaModel *> tagMods;

};

class ProbTagEntry : public TagEntry {
public:
    ProbTagEntry(const KyteaString & str) : TagEntry(str), probs() { }
    ~ProbTagEntry() { }
    
    double incrementProb(const KyteaString & str, int lev);

    void setNumTags(int i) override {
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

    // std::string space(unsigned lev) {
    //     std::ostringstream oss;
    //     while(lev-- > 0)
    //         oss << " ";
    //     return oss.str();
    // }

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
    void checkEqual(const Dictionary<Entry> & rhs) const;

};

}

#endif
