#include <kytea/corpus-io-tokenized.h>
#include <kytea/kytea-struct.h>
#include <kytea/config.h>
#include <kytea/string-util.h>
#include <kytea/kytea-util.h>
#include <kytea/config.h>

#define PROB_TRUE    100.0
#define PROB_FALSE   -100.0
#define PROB_UNKNOWN 0.0

using namespace kytea;
using namespace std;

KyteaSentence * TokenizedCorpusIO::readSentence() {
#ifdef KYTEA_SAFE
    if(out_ || !str_) 
        THROW_ERROR("Attempted to read a sentence from an closed or output object");
#endif
    string s;
    getline(*str_, s);
    if(str_->eof())
        return 0;

    KyteaChar spaceChar = bounds_[0];
    KyteaString ks = util_->mapString(s), buff(ks.length());
    int len = ks.length();
    KyteaSentence * ret = new KyteaSentence();
    int charLen = 0;

    // go through the whole string
    int j = 0, bpos;
    for(j = 0; j < len; j++) {
        // 1) get the word
        bpos = 0;
        for( ; j < len && ks[j] != spaceChar; j++)
            buff[bpos++] = ks[j];
        if(bpos == 0) {
            if(ks[j] == spaceChar)
                continue;
            else
                THROW_ERROR("Empty word at position "<<j<<" in "<<s);
        }
        KyteaString word_str = buff.substr(0,bpos);
        KyteaWord word(word_str, util_->normalize(word_str));
        charLen += bpos;
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

void TokenizedCorpusIO::writeSentence(const KyteaSentence * sent, double conf) {
    const string & wb = util_->showChar(bounds_[0]);
    for(unsigned i = 0; i < sent->words.size(); i++) {
        if(i != 0) *str_ << wb;
        const KyteaWord & w = sent->words[i];
        *str_ << util_->showString(w.surface);
        if(w.getUnknown())
            *str_ << unkTag_;
    }
    *str_ << endl;
}

TokenizedCorpusIO::TokenizedCorpusIO(StringUtil * util, const char* wordBound) : CorpusIO(util), bounds_(1) { 
    bounds_[0] = util_->mapChar(wordBound);
}
TokenizedCorpusIO::TokenizedCorpusIO(const CorpusIO & c, const char* wordBound) : CorpusIO(c), allTags_(false), bounds_(1) { 
    bounds_[0] = util_->mapChar(wordBound);
}
TokenizedCorpusIO::TokenizedCorpusIO(StringUtil * util, const char* file, bool out, const char* wordBound) : CorpusIO(util,file,out), allTags_(false), bounds_(1) { 
    bounds_[0] = util_->mapChar(wordBound);
} 
TokenizedCorpusIO::TokenizedCorpusIO(StringUtil * util, std::iostream & str, bool out, const char* wordBound) : CorpusIO(util,str,out), allTags_(false), bounds_(1) { 
    bounds_[0] = util_->mapChar(wordBound);
}
