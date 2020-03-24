#ifndef CORPUS_IO_PART_H__ 
#define CORPUS_IO_PART_H__ 

#include <kytea/corpus-io.h>
#include <kytea/kytea-string.h>

namespace kytea {

class PartCorpusIO : public CorpusIO {
    
private:

    KyteaString bounds_;

public:
    // PartCorpusIO ctr
    //  util: the string utility to use
    //  unkBound: the delimiter for when the bound is unannotated
    //  skipBound: the delimiter for when annotation of a bound has been skipped
    //  noBound: the delimiter for when no bound exists
    //  hasBound: the delimiter for when a boundary exists
    //  tagBound: the delimiter for when a boundary exists
    //  elemBound: the delimiter for when a boundary exists
    //  escape: the escape character
    PartCorpusIO(StringUtil * util, const char* unkBound = " ", const char* skipBound = "?", const char* noBound = "-", const char* hasBound = "|", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\");
    PartCorpusIO(const CorpusIO & c, const char* unkBound = " ", const char* skipBound = "?", const char* noBound = "-", const char* hasBound = "|", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\");
    PartCorpusIO(StringUtil * util, std::iostream & str, bool out, const char* unkBound = " ", const char* skipBound = "?", const char* noBound = "-", const char* hasBound = "|", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\");
    PartCorpusIO(StringUtil * util, const char* file, bool out, const char* unkBound = " ", const char* skipBound = "?", const char* noBound = "-", const char* hasBound = "|", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\");
    
    KyteaSentence * readSentence() override;
    void writeSentence(const KyteaSentence * sent, double conf = 0.0) override;

};

}

#endif
