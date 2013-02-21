#include <sstream>
#include <cmath>
#include <kytea/corpus-io-prob.h>
#include <kytea/kytea-struct.h>
#include <kytea/config.h>
#include <kytea/string-util.h>
#include <kytea/kytea-util.h>

#define PROB_TRUE    100.0
#define PROB_FALSE   -100.0
#define PROB_UNKNOWN 0.0

using namespace kytea;
using namespace std;

KyteaSentence * ProbCorpusIO::readSentence() {
#ifdef KYTEA_SAFE
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
