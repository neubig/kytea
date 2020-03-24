#ifndef CORPUS_IO_EDA_H__ 
#define CORPUS_IO_EDA_H__ 

#include <kytea/corpus-io.h>

namespace kytea {

class EdaCorpusIO : public CorpusIO {

public:
    EdaCorpusIO(StringUtil * util);
    EdaCorpusIO(const CorpusIO & c);
    EdaCorpusIO(StringUtil * util, const char* file, bool out);
    EdaCorpusIO(StringUtil * util, std::iostream & str, bool out);
    
    KyteaSentence * readSentence() override;
    void writeSentence(const KyteaSentence * sent, double conf = 0.0) override;

protected:
    // The ID of the last sentence printed
    int id_;

};

}

#endif
