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

#define THROW_ERROR(msg) do {                   \
    std::ostringstream oss;                     \
    oss << msg;                                 \
    throw std::runtime_error(oss.str()); }       \
  while (0);

#include <vector>
#include <algorithm>
#include "kytea-string.h"
#include "config.h"

namespace kytea  {

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
        words = Words();
        if(chars.length() != 0) {
            unsigned last = 0;
            for(unsigned i = 0; i < wsConfs.size(); i++) {
                if(wsConfs[i] > confidence) {
                    KyteaWord w(chars.substr(last, i-last+1));
                    words.push_back(w);
                    last = i+1;
                }
            }
            KyteaWord w(chars.substr(last, wsConfs.size()-last+1));
            words.push_back(w);
        }
    }

};

}

// maps for use with various classes
#ifdef HAVE_TR1_UNORDERED_MAP
#   include <tr1/unordered_map>
    typedef std::tr1::unordered_map<std::string, kytea::KyteaChar> StringCharMap;
    typedef std::tr1::unordered_map<kytea::KyteaString,unsigned,kytea::KyteaStringHash> KyteaUnsignedMap;
    typedef std::tr1::unordered_map<kytea::KyteaString,double,kytea::KyteaStringHash>   KyteaDoubleMap;
#elif HAVE_EXT_HASH_MAP
#   include <ext/hash_map>
    namespace __gnu_cxx {
    template <>
    struct hash<std::string> {
        size_t operator() (const std::string& x) const { return hash<const char*>()(x.c_str()); }
    };
    }
    typedef __gnu_cxx::hash_map<std::string, kytea::KyteaChar> StringCharMap;
    typedef __gnu_cxx::hash_map<kytea::KyteaString,unsigned,kytea::KyteaStringHash> KyteaUnsignedMap;
    typedef __gnu_cxx::hash_map<kytea::KyteaString,double,kytea::KyteaStringHash>   KyteaDoubleMap;
#else
#   include <map>
    typedef std::map<std::string, kytea::KyteaChar> StringCharMap;
    typedef std::map<kytea::KyteaString,unsigned> KyteaUnsignedMap;
    typedef std::map<kytea::KyteaString,double>   KyteaDoubleMap;
#endif

#endif
