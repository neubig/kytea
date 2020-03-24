#ifndef CORPUS_IO_RAW_H__ 
#define CORPUS_IO_RAW_H__ 

#include <kytea/corpus-io.h>

namespace kytea {

class RawCorpusIO : public CorpusIO {

public:
    RawCorpusIO(StringUtil * util) : CorpusIO(util) { }
    RawCorpusIO(const CorpusIO & c) : CorpusIO(c) { }
    RawCorpusIO(StringUtil * util, const char* file, bool out) : CorpusIO(util,file,out) { } 
    RawCorpusIO(StringUtil * util, std::iostream & str, bool out) : CorpusIO(util,str,out) { }

    KyteaSentence * readSentence() override;
    void writeSentence(const KyteaSentence * sent, double conf = 0.0) override;

};

}

#endif
