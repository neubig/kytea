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

#include <vector>
#include <algorithm>
#include "kytea-string.h"
#include "config.h"

namespace kytea  {

// KyteaPronunciation
//  a single scored pronunciation candidate
typedef std::pair<KyteaString,double> KyteaPronunciation;
inline bool operator<(const KyteaPronunciation & a, const KyteaPronunciation & b) {
    if(a.second < b.second) return false;
    if(b.second < a.second) return true;
    return a.first < b.first;
}

// KyteaWord
//  a single word, possibly with several pronunciation candidates
class KyteaWord {
public:
    KyteaWord(const KyteaString & s) : surf(s), isCertain(true), unknown(false) { }

    // The surface form of the word
    KyteaString surf;
    // Each of its pronunciations
    std::vector< KyteaPronunciation > prons;
    // Whether the word boundaries are certain
    bool isCertain;
    // Whether this is an unknown word
    bool unknown;

    const KyteaPronunciation * getPron() const { return (prons.size()>0?&prons[0]:0); }
    const std::vector< KyteaPronunciation > & getProns() const { return prons; }
    const KyteaString & getPronSurf() const { return prons[0].first; }
    double getPronConf() const { return prons[0].second; }
    void setPron(const KyteaPronunciation & pron) { 
        prons.resize(1);
        prons[0] = pron;
    }
    void setPronConf(double conf) { prons[0].second = conf; }
    void clearProns() { prons.clear(); }
    void addPron(const KyteaPronunciation & pron) {
        prons.push_back(pron);
    }
    void setUnknown(bool val) { unknown = val; }
    bool getUnknown() const { return unknown; }
    bool hasPron() const { return prons.size() > 0; }

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
