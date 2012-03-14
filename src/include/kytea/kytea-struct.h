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

#ifndef KYTEA_STRUCT_H__
#define KYTEA_STRUCT_H__

#include <stdexcept>
#include <iostream>

#include <vector>
#include <algorithm>
#include "kytea-util.h"
#include "kytea-string.h"
#include "config.h"

// maps for use with various classes
#ifdef HAVE_TR1_UNORDERED_MAP
#if _MSC_VER >=1600
#   include <unordered_map>
#else
#   include <tr1/unordered_map>
#endif
    template <class T>
    class StringMap : public std::tr1::unordered_map<std::string,T> { };
    template <class T>
    class KyteaStringMap : public std::tr1::unordered_map<kytea::KyteaString,T,kytea::KyteaStringHash> { };
#elif HAVE_EXT_HASH_MAP
#   include <ext/hash_map>
    namespace __gnu_cxx {
    template <>
    struct hash<std::string> {
        size_t operator() (const std::string& x) const { return hash<const char*>()(x.c_str()); }
    };
    }
    template <class T>
    class StringMap : public __gnu_cxx::hash_map<std::string,T> { };
    template <class T>
    class KyteaStringMap : public __gnu_cxx::hash_map<kytea::KyteaString,T,kytea::KyteaStringHash> { };
#else
#   include <map>
    template <class T>
    class StringMap : public std::map<std::string,T> { };
    template <class T>
    class KyteaStringMap : public std::map<kytea::KyteaString,T> { };
#endif

namespace kytea  {

// Map equality checking function
template <class T>
void checkMapEqual(const KyteaStringMap<T> & a, const KyteaStringMap<T> & b) {
    if(a.size() != b.size())
        THROW_ERROR("checkMapEqual a.size() != b.size() ("<<a.size()<<", "<<b.size());
    for(typename KyteaStringMap<T>::const_iterator ait = a.begin();
        ait != a.end();
        ait++) {
        typename KyteaStringMap<T>::const_iterator bit = b.find(ait->first);
        if(bit == b.end() || ait->second != bit->second)
            THROW_ERROR("Values don't match in map");
    }
}

// KyteaTag
//  a single scored tag candidate
typedef std::pair<KyteaString,double> KyteaTag;
inline bool operator<(const KyteaTag & a, const KyteaTag & b) {
    if(a.second < b.second) return false;
    if(b.second < a.second) return true;
    return a.first < b.first;
}

// KyteaWord
//  a single word, with multiple lists of candidates for each tag
class KyteaWord {
public:
    KyteaWord(const KyteaString & s) : surf(s), isCertain(true), unknown(false) { }

    // The surface form of the word
    KyteaString surf;
    // Each of its tags
    std::vector< std::vector< KyteaTag > > tags;
    // Whether the word boundaries are certain
    bool isCertain;
    // Whether this is an unknown word
    bool unknown;

    // get a tag for a certain level
    void limitTags(unsigned lev, unsigned lim) {
        if(tags.size() > lev && tags[lev].size() > lim)
            tags[lev].resize(lim);
    }
    const int getNumTags() const { return tags.size(); }
    const KyteaTag * getTag(int lev) const { return (lev<(int)tags.size()&&tags[lev].size()>0) ? &tags[lev][0] : 0; }
    const std::vector< KyteaTag > & getTags(int lev) const { return tags[lev]; }
    const KyteaString & getTagSurf(int lev) const { return tags[lev][0].first; }
    double getTagConf(int lev) const { return tags[lev][0].second; }
    void setTag(int lev, const KyteaTag & tag) { 
        if(lev >= (int)tags.size()) tags.resize(lev+1);
        tags[lev].resize(1);
        tags[lev][0] = tag;
    }
    void setTagConf(int lev, double conf) { tags[lev][0].second = conf; }
    void clearTags(int lev) { if((int)tags.size() > lev) tags[lev].clear(); }
    void addTag(int lev, const KyteaTag & tag) { 
        if(lev >= (int)tags.size()) tags.resize(lev+1);
        tags[lev].push_back(tag);
    }
    void setUnknown(bool val) { unknown = val; }
    bool getUnknown() const { return unknown; }
    bool hasTag(int lev) const { return (int)tags.size() > lev && tags[lev].size() > 0; }

};

// KyteaSentence
//  contains a single sentence with multiple words
class KyteaSentence {

public:

    typedef std::vector<KyteaWord> Words;
    typedef std::vector<double> Floats;

    // the original raw string
    KyteaString chars;
    Floats wsConfs;

    // the string of words
    Words words;

    // constructors
    KyteaSentence() : chars(), wsConfs(0) {
    }
    KyteaSentence(const KyteaString & str) : chars(str), wsConfs(std::max(str.length(),(unsigned)1)-1,0) {
    }

    void refreshWS(double confidence) {
        Words newWords;
        // In order to keep track of new words, use the start and end
        int nextWord = 0, nextEnd = 0, nextStart = -1;
        if(chars.length() != 0) {
            int last = 0, i;
            for(i = 0; i <= (int)wsConfs.size(); i++) {
                double myConf = (i == (int)wsConfs.size()) ? 100.0 : wsConfs[i];
                if(myConf > confidence) {
                    // Catch up to the current word
                    while(nextWord < (int)words.size() && nextEnd < i+1) {
                        nextStart = nextEnd;
                        nextEnd += words[nextWord].surf.length();
                        nextWord++;
                    }
                    // If both the beginning and end match, use the current word
                    if(last == nextStart && i+1 == nextEnd)
                        newWords.push_back(words[nextWord-1]);
                    else {
                        KyteaWord w(chars.substr(last, i-last+1));
                        newWords.push_back(w);
                    }
                    // Update the start of the next word
                    last = i+1;
                }
            }
        }
        words = newWords;
    }

};

}

typedef StringMap<kytea::KyteaChar> StringCharMap;
typedef KyteaStringMap<unsigned> KyteaUnsignedMap;
typedef KyteaStringMap<double>   KyteaDoubleMap;
typedef KyteaStringMap<std::pair<unsigned,unsigned> > TwoCountHash;

#endif
