#include <kytea/corpus-io-eda.h>
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

KyteaSentence * EdaCorpusIO::readSentence() {
    THROW_ERROR("Using EDA format for input is not currently supported");
    return NULL;
}

void EdaCorpusIO::writeSentence(const KyteaSentence * sent, double conf) {
    *str_ << "ID=" << ++id_ << endl;
    for(unsigned i = 0; i < sent->words.size(); i++) {
        const KyteaWord & w = sent->words[i];
        // Find the POS tag
        string tag = "UNK";
        if(w.getNumTags() >= 1) {
            const vector< KyteaTag > & tags = w.getTags(0);
            if(tags.size() > 0)
                tag = util_->showString(tags[0].first);
        }
        // Print
        *str_ << i+1 << " " 
              << i+2 << " "
              << util_->showString(w.surface) << " "
              << tag << " 0" << endl;
    }
    *str_ << endl;
}

EdaCorpusIO::EdaCorpusIO(StringUtil * util) : CorpusIO(util), id_(0) { }
EdaCorpusIO::EdaCorpusIO(const CorpusIO & c) : CorpusIO(c), id_(0) { }
EdaCorpusIO::EdaCorpusIO(StringUtil * util, const char* file, bool out) : CorpusIO(util,file,out), id_(0) { }
EdaCorpusIO::EdaCorpusIO(StringUtil * util, std::iostream & str, bool out) : CorpusIO(util,str,out), id_(0) { }
