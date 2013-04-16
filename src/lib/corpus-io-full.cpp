#include <kytea/corpus-io-full.h>
#include <kytea/kytea-struct.h>
#include <kytea/config.h>
#include <kytea/string-util.h>
#include <kytea/kytea-util.h>
#include "config.h"

#define PROB_TRUE    100.0
#define PROB_FALSE   -100.0
#define PROB_UNKNOWN 0.0

using namespace kytea;
using namespace std;

KyteaSentence * FullCorpusIO::readSentence() {
#ifdef KYTEA_SAFE
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
        KyteaString word_str = buff.substr(0,bpos);
        KyteaWord word(word_str, util_->normalize(word_str));
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
    ret->surface = KyteaString(charLen);
    ret->norm = KyteaString(charLen);
    unsigned pos = 0;
    for(KyteaSentence::Words::const_iterator tit = ret->words.begin(); tit != ret->words.end(); tit++) {
        ret->surface.splice(tit->surface, pos);
        ret->norm.splice(tit->norm, pos);
        unsigned nextPos = pos + tit->surface.length() - 1;
        while(pos++ < nextPos)
            ret->wsConfs.push_back(PROB_FALSE);
        ret->wsConfs.push_back(PROB_TRUE); 
    }
    if(ret->wsConfs.size() > 0)
        ret->wsConfs.pop_back();
    return ret;
}

void FullCorpusIO::writeSentence(const KyteaSentence * sent, double conf) {
    const string & wb = util_->showChar(bounds_[0]), tb = util_->showChar(bounds_[1]), eb = util_->showChar(bounds_[2]);
    for(unsigned i = 0; i < sent->words.size(); i++) {
        if(i != 0) *str_ << wb;
        const KyteaWord & w = sent->words[i];
        if(printWords_) *str_ << util_->showString(w.surface);
        int printed = 0;
        for(int j = 0; j < w.getNumTags(); j++) {
            const vector< KyteaTag > & tags = w.getTags(j);
            if(tags.size() > 0) {
                *str_ << ((printWords_ || printed++ > 0) ? tb : "") << util_->showString(tags[0].first);
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

FullCorpusIO::FullCorpusIO(StringUtil * util, const char* wordBound, const char* tagBound, const char* elemBound, const char* escape) : CorpusIO(util), allTags_(false), bounds_(4), printWords_(true) { 
    bounds_[0] = util_->mapChar(wordBound);
    bounds_[1] = util_->mapChar(tagBound);
    bounds_[2] = util_->mapChar(elemBound);
    bounds_[3] = util_->mapChar(escape);
}
FullCorpusIO::FullCorpusIO(const CorpusIO & c, const char* wordBound, const char* tagBound, const char* elemBound, const char* escape) : CorpusIO(c), allTags_(false), bounds_(4), printWords_(true) { 
    bounds_[0] = util_->mapChar(wordBound);
    bounds_[1] = util_->mapChar(tagBound);
    bounds_[2] = util_->mapChar(elemBound);
    bounds_[3] = util_->mapChar(escape);
}
FullCorpusIO::FullCorpusIO(StringUtil * util, const char* file, bool out, const char* wordBound, const char* tagBound, const char* elemBound, const char* escape) : CorpusIO(util,file,out), allTags_(false), bounds_(4), printWords_(true) { 
    bounds_[0] = util_->mapChar(wordBound);
    bounds_[1] = util_->mapChar(tagBound);
    bounds_[2] = util_->mapChar(elemBound);
    bounds_[3] = util_->mapChar(escape);
} 
FullCorpusIO::FullCorpusIO(StringUtil * util, std::iostream & str, bool out, const char* wordBound, const char* tagBound, const char* elemBound, const char* escape) : CorpusIO(util,str,out), allTags_(false), bounds_(4), printWords_(true) { 
    bounds_[0] = util_->mapChar(wordBound);
    bounds_[1] = util_->mapChar(tagBound);
    bounds_[2] = util_->mapChar(elemBound);
    bounds_[3] = util_->mapChar(escape);
}
