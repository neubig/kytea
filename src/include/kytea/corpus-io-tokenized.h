#ifndef CORPUS_IO_TOKENIZED_H__ 
#define CORPUS_IO_TOKENIZED_H__ 

#include <kytea/corpus-io.h>
#include <kytea/kytea-string.h>

namespace kytea {

// An IO class for corpora that are tokenized, but with no tags
class TokenizedCorpusIO : public CorpusIO {

protected:

    bool allTags_;
    KyteaString bounds_;

public:
    TokenizedCorpusIO(StringUtil * util, const char* wordBound = " ");
    TokenizedCorpusIO(const CorpusIO & c, const char* wordBound = " ");
    TokenizedCorpusIO(StringUtil * util, const char* file, bool out, const char* wordBound = " ");
    TokenizedCorpusIO(StringUtil * util, std::iostream & str, bool out, const char* wordBound = " ");
    
    KyteaSentence * readSentence() override;
    void writeSentence(const KyteaSentence * sent, double conf = 0.0) override;

};

}

#endif
