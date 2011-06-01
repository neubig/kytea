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

#include <kytea/corpus-io.h>
#include <cmath>
#include "config.h"

#define PROB_TRUE    100.0
#define PROB_FALSE   -100.0
#define PROB_UNKNOWN 0.0

using namespace kytea;
using namespace std;

CorpusIO * CorpusIO::createIO(const char* file, Format form, const KyteaConfig & conf, bool output, StringUtil* util) {
    if(form == CORP_FORMAT_FULL)      { return new FullCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_PART) { return new PartCorpusIO(util,file,output,conf.getUnkBound(),conf.getSkipBound(),conf.getNoBound(),conf.getHasBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_PROB) { return new ProbCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_RAW)  { return new RawCorpusIO(util,file,output);  }
    else
        THROW_ERROR("Illegal Output Format");
}

CorpusIO * CorpusIO::createIO(iostream & file, Format form, const KyteaConfig & conf, bool output, StringUtil* util) {
    if(form == CORP_FORMAT_FULL)      { return new FullCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_PART) { return new PartCorpusIO(util,file,output,conf.getUnkBound(),conf.getSkipBound(),conf.getNoBound(),conf.getHasBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_PROB) { return new ProbCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_RAW)  { return new RawCorpusIO(util,file,output);  }
    else 
        THROW_ERROR("Illegal Output Format");
}


KyteaSentence * FullCorpusIO::readSentence() {
#ifdef KYTEA_IO_SAFE
    if(out_ || !str_) 
        THROW_ERROR("Attempted to read a sentence from an closed or output object");
#endif
    string s;
    getline(*str_, s);
    if(str_->eof())
        return 0;

    KyteaChar spaceChar = bounds_[0], slashChar = bounds_[1], ampChar = bounds_[2], bsChar = bounds_[3];
    KyteaString ks = util_->mapString(s), buff(ks.length());
    int len = ks.length();
    KyteaSentence * ret = new KyteaSentence();
    int charLen = 0;

    // go through the whole string
    int j = 0, bpos, lev;
    for(j = 0; j < len; j++) {
        // 1) get the word
        bpos = 0;
        for( ; j < len && ks[j] != spaceChar && ks[j] != slashChar; j++) {
            if(ks[j] == ampChar) {
                THROW_ERROR("Illegal tag separator in word position at "<<s);
            } else if(ks[j] == bsChar && ++j == len) {
                THROW_ERROR("Illegal trailing escape character at "<<s);
            }
            buff[bpos++] = ks[j];
        }
        if(bpos == 0) {
            if(ks[j] == spaceChar)
                continue;
            else
                THROW_ERROR("Empty word at position "<<j<<" in "<<s);
        }
        KyteaWord word(buff.substr(0,bpos));
        charLen += bpos;
        // 2) get the tags
        lev = -1;
        while(j < len && ks[j] != spaceChar) {
            if(ks[j] == slashChar) lev++;
            bpos = 0;
            for(++j ; j < len && ks[j] != spaceChar && ks[j] != slashChar && ks[j] != ampChar; j++) {
                if(ks[j] == bsChar && ++j == len)
                    THROW_ERROR("Illegal trailing escape character at "<<s);
                buff[bpos++] = ks[j];
            }
            if(bpos != 0)
                word.addTag(lev,KyteaTag(buff.substr(0,bpos),PROB_TRUE));
        }
        ret->words.push_back(word);
    }
     
    // make the character/ws string
    ret->chars = KyteaString(charLen);
    unsigned pos = 0;
    for(KyteaSentence::Words::const_iterator tit = ret->words.begin(); tit != ret->words.end(); tit++) {
        ret->chars.splice(tit->surf, pos);
        unsigned nextPos = pos + tit->surf.length() - 1;
        while(pos++ < nextPos)
            ret->wsConfs.push_back(PROB_FALSE);
        ret->wsConfs.push_back(PROB_TRUE); 
    }
    if(ret->wsConfs.size() > 0)
        ret->wsConfs.pop_back();
    // // DEBUG START
    // cout << "words. ";
    // for(unsigned i = 0; i < ret->words.size(); i++) {
    //     if(i != 0) cout << " ||| ";
    //     cout << util_->showString(ret->words[i].surf);
    // }
    // cout << endl;
    // // DEBUG END
    return ret;
}

void FullCorpusIO::writeSentence(const KyteaSentence * sent, double conf) {
    const string & wb = util_->showChar(bounds_[0]), tb = util_->showChar(bounds_[1]), eb = util_->showChar(bounds_[2]);
    for(unsigned i = 0; i < sent->words.size(); i++) {
        if(i != 0) *str_ << wb;
        const KyteaWord & w = sent->words[i];
        *str_ << util_->showString(w.surf);
        for(int j = 0; j < w.getNumTags(); j++) {
            const vector< KyteaTag > & tags = w.getTags(j);
            if(tags.size() > 0) {
                *str_ << tb << util_->showString(tags[0].first);
                if(allTags_) 
                    for(unsigned k = 1; k < tags.size(); k++) 
                        *str_ << eb << util_->showString(tags[k].first);
            }
        }
        if(w.getUnknown())
            *str_ << unkTag_;
    }
    *str_ << endl;
}

KyteaString mapList(const list<KyteaChar> & lst) {
    KyteaString ret(lst.size());
    unsigned pos = 0;
    for(list<KyteaChar>::const_iterator it = lst.begin(); it != lst.end(); it++)
        ret[pos++] = *it;
    return ret;
}
KyteaSentence * PartCorpusIO::readSentence() {
#ifdef KYTEA_IO_SAFE
    if(out_ || !str_) 
        THROW_ERROR("Attempted to read a sentence from an closed or output object");
#endif
    string s;
    getline(*str_, s);
    if(str_->eof())
        return 0;
    KyteaString ks = util_->mapString(s), buff(ks.length());
    KyteaChar ukBound = bounds_[0], skipBound = bounds_[1], noBound = bounds_[2], 
        hasBound = bounds_[3], slashChar = bounds_[4], elemChar = bounds_[5], 
        escapeChar = bounds_[6];
    KyteaSentence * ret = new KyteaSentence();

    int len = ks.length(), charLen = 0;
    for(int j = 0; j < len; j++) {
        int bpos = 0;
        bool cert = true;
        // read in a word
        for( ; j < len; j++) {
            if(ks[j] == ukBound || ks[j] == skipBound || ks[j] == noBound || ks[j] == hasBound || ks[j] == slashChar || ks[j] == elemChar)
                THROW_ERROR("Misplaced character '"<<util_->showChar(ks[j])<<"' in "<<s);
            if(ks[j] == escapeChar && ++j >= len)
                THROW_ERROR("Misplaced escape at the end of "<<s);
            buff[bpos++] = ks[j++];
            if(j >= len || ks[j] == slashChar || ks[j] == hasBound) 
                break;
            else if(ks[j] == ukBound || ks[j] == skipBound) {
                ret->wsConfs.push_back(PROB_UNKNOWN);
                cert = false;
            } else if(ks[j] != noBound) {
                THROW_ERROR("Misplaced character '"<<util_->showChar(ks[j])<<"' in "<<s);
            } else
                ret->wsConfs.push_back(PROB_FALSE);
        }
        KyteaWord word(buff.substr(0,bpos));
        charLen += bpos;
        word.isCertain = cert;
        bpos = 0;
        // read in the tags
        int lev = -1;
        while(j < len && ks[j] != hasBound) {
            if(ks[j] == slashChar) lev++;
            bpos = 0;
            for(++j ; j < len && ks[j] != hasBound && ks[j] != slashChar && ks[j] != elemChar; j++) {
                if(ks[j] == escapeChar && ++j == len)
                    THROW_ERROR("Illegal trailing escape character at "<<s);
                buff[bpos++] = ks[j];
            }
            if(bpos == 0)
                THROW_ERROR("Empty tag at position "<<j<<" in "<<s);
            word.addTag(lev,KyteaTag(buff.substr(0,bpos),PROB_TRUE));
        }
        if(j != len)
            ret->wsConfs.push_back(PROB_TRUE);
        ret->words.push_back(word);
    }

    // make the character/ws string
    ret->chars = KyteaString(charLen);
    unsigned pos = 0;
    for(KyteaSentence::Words::const_iterator tit = ret->words.begin(); tit != ret->words.end(); tit++) {
        ret->chars.splice(tit->surf, pos);
        pos += tit->surf.length();
    }

    return ret;
}

void PartCorpusIO::writeSentence(const KyteaSentence * sent, double conf)  {
    unsigned curr = 0;
    const string & ukBound = util_->showChar(bounds_[0]), & skipBound = util_->showChar(bounds_[1]), 
        &noBound = util_->showChar(bounds_[2]), &hasBound = util_->showChar(bounds_[3]), 
        &slashChar = util_->showChar(bounds_[4]), &elemChar = util_->showChar(bounds_[5]);
    for(unsigned i = 0; i < sent->words.size(); i++) {
        const KyteaWord & w = sent->words[i];
        string sepType = ukBound;
        for(unsigned j = 0; j < w.surf.length(); ) {
            *str_ << util_->showChar(sent->chars[curr]);
            if(curr == sent->wsConfs.size()) sepType = skipBound;
            else if(sent->wsConfs[curr] > conf) sepType = hasBound;
            else if(sent->wsConfs[curr] < conf*-1) sepType = noBound;
            else sepType = ukBound;
            if(++j != w.surf.length())
                *str_ << sepType;
            curr++;
        }
        for(int j = 0; j < w.getNumTags(); j++) {
            const vector<KyteaTag> & tags = w.getTags(j);
            for(int k = 0; k < (int)tags.size(); k++)
                if(tags[k].second > conf)
                    *str_ << (k==0?slashChar:elemChar) << util_->showString(tags[k].first);
        }
        if(w.getUnknown())
            *str_ << unkTag_;
        if(sepType != skipBound)
            *str_ << sepType;
    }
    *str_ << endl;
}

KyteaSentence * ProbCorpusIO::readSentence() {
#ifdef KYTEA_IO_SAFE
    if(out_ || !str_) 
        THROW_ERROR("Attempted to read a sentence from an closed or output object");
#endif
    KyteaSentence* ret = FullCorpusIO::readSentence();
    if(ret == 0)
        return 0;
    // get the ws confidences
    string s;
    getline(*str_, s);
    istringstream wsiss(s);
    KyteaSentence::Floats::iterator wsit = ret->wsConfs.begin();
    while((wsiss >> s) && (wsit != ret->wsConfs.end())) {
        *wsit = util_->parseFloat(s.c_str());
        wsit++;
    }
    if(wsiss.good() || wsit != ret->wsConfs.end()) {
        THROW_ERROR("Bad number of WS confidences in a probability file");
    }
    // get the pe confidences
    for(int i = 0; i < getNumTags(); i++) {
        getline(*str_, s);
        istringstream peiss(s);
        KyteaSentence::Words::iterator peit = ret->words.begin();
        while((peiss >> s) && (peit != ret->words.end())) {
            if(peit->getTag(i))
                peit->setTagConf(i,util_->parseFloat(s.c_str()));
            peit++;
        }
        if(peiss.good() || peit != ret->words.end()) {
            THROW_ERROR("Bad number of PE confidences in a probability file");
        }
    }
    // get the separator line
    getline(*str_, s);
    if(s.length())
        THROW_ERROR("Badly formatted probability file (no white-space between sentences)");

    return ret;
}

void ProbCorpusIO::writeSentence(const KyteaSentence * sent, double conf)  {
    FullCorpusIO::writeSentence(sent, conf);
    const string & space = util_->showChar(bounds_[0]), &amp = util_->showChar(bounds_[2]);
    for(unsigned i = 0; i < sent->wsConfs.size(); i++) {
        if(i != 0) *str_ << space;
        *str_ << abs(sent->wsConfs[i]);
    }
    *str_ << endl;
    for(int k = 0; k < getNumTags(); k++) {
        if(getDoTag(k)) {
            for(unsigned i = 0; i < sent->words.size(); i++) {
                if(i != 0) *str_ << space;
                const vector< KyteaTag > & tags = sent->words[i].getTags(k);
                if(tags.size() > 0) {
                    *str_ << tags[0].second;
                    if(allTags_)
                        for(unsigned j = 1; j < tags.size(); j++)
                            *str_ << amp << tags[j].second;
                } else
                    *str_ << 0;
            }
            *str_ << endl;
        }
    }
    *str_ << endl;
}

KyteaSentence * RawCorpusIO::readSentence() {
#ifdef KYTEA_IO_SAFE
    if(out_ || !str_) 
        THROW_ERROR("Attempted to read a sentence from an closed or output object");
#endif
    string s;
    getline(*str_, s);
    if(str_->eof())
        return 0;
    KyteaSentence * ret = new KyteaSentence();
    ret->chars = util_->mapString(s);
    if(ret->chars.length() != 0)
        ret->wsConfs.resize(ret->chars.length()-1,0);
    return ret;
}

void RawCorpusIO::writeSentence(const KyteaSentence * sent, double conf)  {
    *str_ << util_->showString(sent->chars) << endl;
}
