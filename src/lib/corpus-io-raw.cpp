#include <kytea/corpus-io-raw.h>
#include <kytea/kytea-struct.h>
#include <kytea/config.h>
#include <kytea/string-util.h>
#include <kytea/kytea-util.h>

#define PROB_TRUE    100.0
#define PROB_FALSE   -100.0
#define PROB_UNKNOWN 0.0

using namespace kytea;
using namespace std;

KyteaSentence * RawCorpusIO::readSentence() {
#ifdef KYTEA_SAFE
    if(out_ || !str_) 
        THROW_ERROR("Attempted to read a sentence from an closed or output object");
#endif
    string s;
    getline(*str_, s);
    if(str_->eof())
        return 0;
    KyteaSentence * ret = new KyteaSentence();
    ret->surface = util_->mapString(s);
    ret->norm = util_->normalize(ret->surface);
    if(ret->surface.length() != 0)
        ret->wsConfs.resize(ret->surface.length()-1,0);
    return ret;
}

void RawCorpusIO::writeSentence(const KyteaSentence * sent, double conf)  {
    *str_ << util_->showString(sent->surface) << endl;
}
