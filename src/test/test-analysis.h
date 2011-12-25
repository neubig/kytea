#ifndef TEST_ANALYSIS__
#define TEST_ANALYSIS__

#include "test-base.h"
#include <kytea/corpus-io.h>

namespace kytea {

class TestAnalysis : public TestBase {

private:

    Kytea * kytea;
    StringUtil * util;

public:

    TestAnalysis() {
        // Print the corpus
        const char* toy_text = "これ/代名詞/これ は/助詞/は 学習/名詞/がくしゅう データ/名詞/でーた で/助動詞/で す/語尾/す 。/補助記号/。\n"
                               "どうぞ/副詞/どうぞ モデル/名詞/もでる を/助詞/を 学習/名詞/がくしゅう し/動詞/し て/助詞/て くださ/動詞/くださ い/語尾/い ！/補助記号/！\n";
        ofstream ofs("/tmp/kytea-toy-corpus.txt"); 
        ofs << toy_text; ofs.close();
        // Train the KyTea model
        const char* toy_cmd[7] = {"", "-model", "/tmp/kytea-toy-model.bin", "-full", "/tmp/kytea-toy-corpus.txt", "-global", "1"};
        KyteaConfig * config = new KyteaConfig;
        config->setDebug(0);
        config->setOnTraining(true);
        config->parseTrainCommandLine(7, toy_cmd);
        kytea = new Kytea(config);
        kytea->trainAll();
        util = kytea->getStringUtil();
    }

    ~TestAnalysis() {
        delete kytea;
    }

    int testWordSegmentation() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaSentence sentence(util->mapString("これはデータです。"));
        kytea->calculateWS(sentence);
        // Make the correct words
        KyteaString::Tokens toks = util->mapString("これ は データ で す 。").tokenize(util->mapString(" "));
        return checkWordSeg(sentence,toks,util);
    }

    int testPartialSegmentation() {
        // Read in a partially annotated sentence
        stringstream instr;
        instr << "こ|れ-は デ ー タ で-す 。" << endl;
        PartCorpusIO io(util, instr, false);
        KyteaSentence * sent = io.readSentence();
        kytea->calculateWS(*sent);
        // Make the correct words
        KyteaString::Tokens toks = util->mapString("こ れは データ です 。").tokenize(util->mapString(" "));
        int ok = checkWordSeg(*sent,toks,util);
        delete sent;
        return ok;
    }

    bool runTest() {
        int done = 0, succeeded = 0;
        done++; cout << "testWordSegmentation()" << endl; if(testWordSegmentation()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testPartialSegmentation()" << endl; if(testPartialSegmentation()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "Finished with "<<succeeded<<"/"<<done<<" tests succeeding"<<endl;
        return done == succeeded;
    }

};

}



#endif
