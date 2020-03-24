#ifndef CORPUS_IO_PROB_H__ 
#define CORPUS_IO_PROB_H__ 

#include <kytea/corpus-io-full.h>

namespace kytea {

class ProbCorpusIO : public FullCorpusIO {

public:
    ProbCorpusIO(StringUtil * util, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : FullCorpusIO(util,wordBound,tagBound,elemBound,escape) { allTags_ = true; }
    ProbCorpusIO(const CorpusIO & c, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : FullCorpusIO(c,wordBound,tagBound,elemBound,escape) { allTags_ = true; }
    ProbCorpusIO(StringUtil * util, const char* file, bool out, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : FullCorpusIO(util,file,out,wordBound,tagBound,elemBound,escape) { allTags_ = true; } 
    ProbCorpusIO(StringUtil * util, std::iostream & str, bool out, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : FullCorpusIO(util,str,out,wordBound,tagBound,elemBound,escape) { allTags_ = true; }

    KyteaSentence * readSentence() override;
    void writeSentence(const KyteaSentence * sent, double conf = 0.0) override;

};

}

#endif
