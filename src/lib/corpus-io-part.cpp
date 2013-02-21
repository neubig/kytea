#include <kytea/corpus-io-part.h>
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

KyteaSentence * PartCorpusIO::readSentence() {
#ifdef KYTEA_SAFE
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
        KyteaString word_str = buff.substr(0,bpos);
        KyteaWord word(word_str, util_->normalize(word_str));
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
            if(bpos != 0)
                word.addTag(lev,KyteaTag(buff.substr(0,bpos),PROB_TRUE));
        }
        if(j != len)
            ret->wsConfs.push_back(PROB_TRUE);
        ret->words.push_back(word);
    }

    // make the character/ws string
    ret->surface = KyteaString(charLen);
    ret->norm = KyteaString(charLen);
    unsigned pos = 0;
    for(KyteaSentence::Words::const_iterator tit = ret->words.begin(); tit != ret->words.end(); tit++) {
        ret->surface.splice(tit->surface, pos);
        ret->norm.splice(tit->norm, pos);
        pos += tit->surface.length();
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
        for(unsigned j = 0; j < w.surface.length(); ) {
            *str_ << util_->showChar(sent->surface[curr]);
            if(curr == sent->wsConfs.size()) sepType = skipBound;
            else if(sent->wsConfs[curr] > conf) sepType = hasBound;
            else if(sent->wsConfs[curr] < conf*-1) sepType = noBound;
            else sepType = ukBound;
            if(++j != w.surface.length())
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

PartCorpusIO::PartCorpusIO(StringUtil * util, const char* unkBound, const char* skipBound, const char* noBound, const char* hasBound, const char* tagBound, const char* elemBound, const char* escape) : CorpusIO(util), bounds_(7) { 
    bounds_[0] = util_->mapChar(unkBound);
    bounds_[1] = util_->mapChar(skipBound);
    bounds_[2] = util_->mapChar(noBound);
    bounds_[3] = util_->mapChar(hasBound);
    bounds_[4] = util_->mapChar(tagBound);
    bounds_[5] = util_->mapChar(elemBound);
    bounds_[6] = util_->mapChar(escape);
}
PartCorpusIO::PartCorpusIO(const CorpusIO & c, const char* unkBound, const char* skipBound, const char* noBound, const char* hasBound, const char* tagBound, const char* elemBound, const char* escape) : CorpusIO(c), bounds_(7) { 
    bounds_[0] = util_->mapChar(unkBound);
    bounds_[1] = util_->mapChar(skipBound);
    bounds_[2] = util_->mapChar(noBound);
    bounds_[3] = util_->mapChar(hasBound);
    bounds_[4] = util_->mapChar(tagBound);
    bounds_[5] = util_->mapChar(elemBound);
    bounds_[6] = util_->mapChar(escape);
}
PartCorpusIO::PartCorpusIO(StringUtil * util, std::iostream & str, bool out, const char* unkBound, const char* skipBound, const char* noBound, const char* hasBound, const char* tagBound, const char* elemBound, const char* escape) : CorpusIO(util,str,out), bounds_(7) { 
    bounds_[0] = util_->mapChar(unkBound);
    bounds_[1] = util_->mapChar(skipBound);
    bounds_[2] = util_->mapChar(noBound);
    bounds_[3] = util_->mapChar(hasBound);
    bounds_[4] = util_->mapChar(tagBound);
    bounds_[5] = util_->mapChar(elemBound);
    bounds_[6] = util_->mapChar(escape);
}
PartCorpusIO::PartCorpusIO(StringUtil * util, const char* file, bool out, const char* unkBound, const char* skipBound, const char* noBound, const char* hasBound, const char* tagBound, const char* elemBound, const char* escape) : CorpusIO(util,file,out), bounds_(7) { 
    bounds_[0] = util_->mapChar(unkBound);
    bounds_[1] = util_->mapChar(skipBound);
    bounds_[2] = util_->mapChar(noBound);
    bounds_[3] = util_->mapChar(hasBound);
    bounds_[4] = util_->mapChar(tagBound);
    bounds_[5] = util_->mapChar(elemBound);
    bounds_[6] = util_->mapChar(escape);
}
