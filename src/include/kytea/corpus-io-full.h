#ifndef CORPUS_IO_FULL_H__ 
#define CORPUS_IO_FULL_H__ 

#include <kytea/corpus-io.h>
#include <kytea/kytea-string.h>

namespace kytea {

class FullCorpusIO : public CorpusIO {

protected:

    bool allTags_;
    KyteaString bounds_;
    bool printWords_;

public:
    FullCorpusIO(StringUtil * util, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\");
    FullCorpusIO(const CorpusIO & c, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\");
    FullCorpusIO(StringUtil * util, const char* file, bool out, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\");
    FullCorpusIO(StringUtil * util, std::iostream & str, bool out, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\");
    
    KyteaSentence * readSentence() override;
    void writeSentence(const KyteaSentence * sent, double conf = 0.0) override;
    void setPrintWords(bool printWords) { printWords_ = printWords; }

};

}

#endif
