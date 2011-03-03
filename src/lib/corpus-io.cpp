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
    else {
        throw runtime_error("Illegal Output Format");
    }
}

CorpusIO * CorpusIO::createIO(iostream & file, Format form, const KyteaConfig & conf, bool output, StringUtil* util) {
    if(form == CORP_FORMAT_FULL)      { return new FullCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_PART) { return new PartCorpusIO(util,file,output,conf.getUnkBound(),conf.getSkipBound(),conf.getNoBound(),conf.getHasBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_PROB) { return new ProbCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_RAW)  { return new RawCorpusIO(util,file,output);  }
    else {
        throw runtime_error("Illegal Output Format");
    }
}


KyteaSentence * FullCorpusIO::readSentence() {
#ifdef KYTEA_IO_SAFE
    if(out_ || !str_) 
        throw runtime_error("Attempted to read a sentence from an closed or output object");
#endif
    string s;
    getline(*str_, s);
    if(str_->eof())
        return 0;
    KyteaString::Tokens toks = util_->mapString(s).tokenize(bounds_,true);

    unsigned charLen = 0, myLen;
    KyteaChar spaceChar = bounds_[0], slashChar = bounds_[1], ampChar = bounds_[2], bsChar = bounds_[3];
    KyteaSentence * ret = new KyteaSentence();
    string old;
    // find the words
    // // DEBUG START
    // cout << "reading sentence: ";
    // for(list<KyteaString>::const_iterator tit = toks.begin(); tit != toks.end(); tit++) {
    //     if(tit != toks.begin()) cout << " ||| ";
    //     cout << util_->showString(*tit);
    // }
    // cout << endl;
    // // DEBUG END
    for(list<KyteaString>::const_iterator tit = toks.begin(); tit != toks.end(); ) {
        // expect the start of a word here
        if((*tit)[0] == spaceChar) { tit++; continue; } // skip repeated spaces
        else if((*tit)[0] == slashChar) {
            cerr << s << endl;
            throw runtime_error("Misplaced slash in annotated corpus");
        }
        // get the length
        myLen = tit->length();
        // check for escapes
        KyteaString surf;
        while(tit != toks.end() && (*tit)[0] != spaceChar && 
                (*tit)[0] != slashChar && (*tit)[0] != ampChar) {
            if((*tit)[0] == bsChar) {
                if(++tit == toks.end()) {
                    cerr << s << endl;
                    throw runtime_error("Escape character \\ placed before end of line");
                }
            }
            surf = surf + *(tit++);
        }
        charLen += surf.length();
        KyteaWord word(surf);
        while(tit != toks.end() && ((*tit)[0] == slashChar || (*tit)[0] == ampChar)) {
            if((((*tit)[0] == ampChar) != word.hasPron()) || ++tit == toks.end()) {
                cerr << s << endl;
                throw runtime_error("Misplaced tag delimiter in annotated corpus");
            }
            word.addPron(KyteaPronunciation(*tit,PROB_TRUE));
            tit++;
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
        const vector< KyteaPronunciation > & prons = w.getProns();
        if(prons.size() > 0) {
            *str_ << tb << util_->showString(prons[0].first);
            if(allProns_) 
                for(unsigned j = 1; j < prons.size(); j++) 
                    *str_ << eb << util_->showString(prons[j].first);
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
        throw runtime_error("Attempted to read a sentence from an closed or output object");
#endif
    string s;
    getline(*str_, s);
    if(str_->eof())
        return 0;
    ostringstream errbuff;
    KyteaString input = util_->mapString(s);
    KyteaChar ukBound = bounds_[0], skipBound = bounds_[1], noBound = bounds_[2], 
        hasBound = bounds_[3], slashChar = bounds_[4], // elemChar = bounds_[5], 
        escapeChar = bounds_[6];
    unsigned pos = 0;
    bool isCertain = true;
    list<KyteaChar> buff, wordBuff;
    KyteaSentence * ret = new KyteaSentence();
    while(pos < input.length()) {
        // handle escape characters
        if(input[pos] == escapeChar) {
            if(++pos == input.length()) {
                errbuff << "Trailing escape character in line: " << util_->showString(input);
                throw runtime_error(errbuff.str());
            }
        }
        // add the input
        wordBuff.push_back(input[pos]);
        buff.push_back(input[pos++]);
        KyteaChar currChar = (pos == input.length()?hasBound:input[pos]);
        if(currChar == slashChar || currChar == hasBound) {
            KyteaWord word(mapList(wordBuff));
            wordBuff.clear();
            word.isCertain = isCertain;
            if(currChar == slashChar) {
                unsigned firstPos = ++pos;
                while(pos < input.length() && input[pos] != hasBound)
                    pos++;
                word.setPron(KyteaPronunciation(input.substr(firstPos, pos-firstPos),PROB_TRUE));
            }
            ret->words.push_back(word);
            isCertain = true;
        }
        if(pos < input.length()) {
            if(input[pos] == ukBound || input[pos] == skipBound) {
                isCertain = false;
                ret->wsConfs.push_back(PROB_UNKNOWN);
            }
            else if(input[pos] == noBound)
                ret->wsConfs.push_back(PROB_FALSE);
            else if(input[pos] == hasBound)   
                ret->wsConfs.push_back(PROB_TRUE);
            else {
                cerr << util_->showString(input) << endl;
                errbuff << "Malformed partially annotated corpus (illegal bound '" << util_->showChar(input[pos]) << ")";
                throw runtime_error(errbuff.str());
            }
        }
        pos++;
    }
    ret->chars = mapList(buff);

    return ret;
}

void PartCorpusIO::writeSentence(const KyteaSentence * sent, double conf)  {
    unsigned curr = 0;
    const string & ukBound = util_->showChar(bounds_[0]), & skipBound = util_->showChar(bounds_[1]), 
        &noBound = util_->showChar(bounds_[2]), &hasBound = util_->showChar(bounds_[3]), 
        &slashChar = util_->showChar(bounds_[4]);
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
        if(w.getPron() && w.getPronConf() > conf)
            *str_ << slashChar << util_->showString(w.getPronSurf());
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
        throw runtime_error("Attempted to read a sentence from an closed or output object");
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
        throw runtime_error("Bad number of WS confidences in a probability file");
    }
    // get the pe confidences
    getline(*str_, s);
    istringstream peiss(s);
    KyteaSentence::Words::iterator peit = ret->words.begin();
    while((peiss >> s) && (peit != ret->words.end())) {
        if(peit->getPron())
            peit->setPronConf(util_->parseFloat(s.c_str()));
        peit++;
    }
    if(peiss.good() || peit != ret->words.end()) {
        throw runtime_error("Bad number of PE confidences in a probability file");
    }
    // get the separator line
    getline(*str_, s);
    if(s.length())
        throw runtime_error("Badly formatted probability file (no white-space between sentences)");

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
    for(unsigned i = 0; i < sent->words.size(); i++) {
        if(i != 0) *str_ << space;
        const vector< KyteaPronunciation > & prons = sent->words[i].getProns();
        if(prons.size() > 0) {
            *str_ << prons[0].second;
            if(allProns_)
                for(unsigned j = 1; j < prons.size(); j++)
                    *str_ << amp << prons[j].second;
        } else
            *str_ << 0;
    }
    *str_ << endl << endl;
}

KyteaSentence * RawCorpusIO::readSentence() {
#ifdef KYTEA_IO_SAFE
    if(out_ || !str_) 
        throw runtime_error("Attempted to read a sentence from an closed or output object");
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
