#ifndef TEST_CORPUSIO__
#define TEST_CORPUSIO__

#include <kytea/corpus-io.h>
#include "test-base.h"

namespace kytea {

class TestCorpusIO : public TestBase {

private:

    StringUtilUtf8 * util;

public:

    TestCorpusIO() {
        util = new StringUtilUtf8;
    }

    ~TestCorpusIO() {
        delete util;    
    }

    int testWordSegConf() {
        // Build the string
        stringstream instr;
        instr << "こ|れ-は デ ー タ で-す 。" << endl;
        PartCorpusIO io(util, instr, false);
        KyteaSentence * sent = io.readSentence();
        // Build the expectations
        vector<double> exp(8,0.0);
        exp[0] = 100; exp[1] = -100; exp[6] = -100;
        return checkVector(exp, sent->wsConfs); 
    }

    bool runTest() {
        int done = 0, succeeded = 0;
        done++; cout << "testWordSegConf()" << endl; if(testWordSegConf()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "Finished with "<<succeeded<<"/"<<done<<" tests succeeding"<<endl;
        return done == succeeded;
    }

};

}



#endif
