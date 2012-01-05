#include "test-base.h"

#ifndef TEST_SENTENCE_H__
#define TEST_SENTENCE_H__

namespace kytea {

class TestSentence : public TestBase {

private:

    StringUtilUtf8 * util;

public:

    TestSentence() {
        util = new StringUtilUtf8;
    }

    ~TestSentence() {
        delete util;    
    }

    int testRefreshWS() {
        // Build the string
        stringstream instr;
        instr << "これ は データ/名詞 で/助動詞 す/語尾 。" << endl;
        FullCorpusIO io(util, instr, false);
        KyteaSentence * sent = io.readSentence();
        // Refresh the word segmentation
        sent->wsConfs[6] = -100;
        sent->refreshWS(1);
        // Check to make sure it matches our expectation
        KyteaString::Tokens words = util->mapString("これ は データ です 。").tokenize(util->mapString(" "));
        KyteaString::Tokens tags = util->mapString("NONE NONE 名詞 NONE NONE").tokenize(util->mapString(" "));
        bool wordsOK = checkWordSeg(*sent,words,util); 
        bool tagsOK = checkTags(*sent,tags,0,util); 
        delete sent;
        return wordsOK && tagsOK;
    }

    bool runTest() {
        int done = 0, succeeded = 0;
        done++; cout << "testRefreshWS()" << endl; if(testRefreshWS()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestSentence Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

};

}

#endif
