#ifndef TEST_CORPUSIO_SJIS__
#define TEST_CORPUSIO_SJIS__

#include <kytea/corpus-io.h>
#include "test-base.h"

namespace kytea {

class TestCorpusIOSjis : public TestBase {

private:

    StringUtilSjis * util;

public:

    TestCorpusIOSjis() {
        util = new StringUtilSjis;
    }

    ~TestCorpusIOSjis() {
        delete util;    
    }

    int testWordSegConf() {
        // Build the string
        stringstream instr;
        // instr << "こ|れ-は デ ー タ で-す 。" << endl;
        instr << "\x82\xb1\x7c\x82\xea\x2d\x82\xcd\x20\x83\x66\x20\x81\x5b\x20\x83\x5e\x20\x82\xc5\x2d\x82\xb7\x20\x81\x42" << endl;
        PartCorpusIO io(util, instr, false);
        KyteaSentence * sent = io.readSentence();
        // Build the expectations
        vector<double> exp(8,0.0);
        exp[0] = 100; exp[1] = -100; exp[6] = -100;
        bool ret = checkVector(exp, sent->wsConfs); 
        delete sent;
        return ret;
    }

    int testPartEmptyLines() {
        // Build the string
        stringstream instr;
        instr << "" << endl;
        PartCorpusIO io(util, instr, false);
        KyteaSentence * sent = io.readSentence();
        // Build the expectations
        vector<double> exp(0,0.0);
        bool ret = checkVector(exp, sent->wsConfs); 
        delete sent;
        return ret;
    }

    int testPartEmptyTag() {
        // Build the string
        stringstream instr;
        // instr << "こ-れ//これ" << endl;
        instr << "\x82\xb1\x2d\x82\xea\x2f\x2f\x82\xb1\x82\xea" << endl;
        PartCorpusIO io(util, instr, false);
        KyteaSentence * sent = io.readSentence();
        int ret = 1;
        if(sent->words.size() != 1) {
            cerr << "Sentence size " << sent->words.size() << " != 1" << endl;
            ret = 0;
        }
        delete sent;
        return ret;
    }

    int testFullTagConf() {
        // Build the string
        stringstream instr;
        // instr << "こ-れ/名詞 は/助詞 データ/名詞 で/助動詞 す/語尾 。/補助記号" << endl;
        instr << "\x82\xb1\x2d\x82\xea\x2f\x96\xbc\x8e\x8c\x20\x82\xcd\x2f\x8f\x95\x8e\x8c\x20\x83\x66\x81\x5b\x83\x5e\x2f\x96\xbc\x8e\x8c\x20\x82\xc5\x2f\x8f\x95\x93\xae\x8e\x8c\x20\x82\xb7\x2f\x8c\xea\x94\xf6\x20\x81\x42\x2f\x95\xe2\x8f\x95\x8b\x4c\x8d\x86" << endl;
        FullCorpusIO io(util, instr, false);
        KyteaSentence * sent = io.readSentence();
        // Build the expectations
        if(sent->words.size() != 6)
            THROW_ERROR("sent->words size doesn't match 5 " << sent->words.size());
        bool ret = true;
        for(int i = 0; i < 6; i ++) {
            if(sent->words[i].tags[0][0].second != 100.0) {
                cerr << "Bad confidence for tag " << i << ": " << sent->words[i].tags[0][0].second << endl;
                ret = false;
            }
        }
        delete sent;
        return ret;
    }

    int testLastValue() {
        // string confident_text = "これ/代名詞/これ は/助詞/は 信頼/名詞/しんらい 度/接尾辞/ど の/助詞/の 高/形容詞/たか い/語尾/い 入力/名詞/にゅうりょく で/助動詞/で す/語尾/す 。/補助記号/。\n";
        string confident_text = "\x82\xb1\x82\xea\x2f\x91\xe3\x96\xbc\x8e\x8c\x2f\x82\xb1\x82\xea\x20\x82\xcd\x2f\x8f\x95\x8e\x8c\x2f\x82\xcd\x20\x90\x4d\x97\x8a\x2f\x96\xbc\x8e\x8c\x2f\x82\xb5\x82\xf1\x82\xe7\x82\xa2\x20\x93\x78\x2f\x90\xda\x94\xf6\x8e\xab\x2f\x82\xc7\x20\x82\xcc\x2f\x8f\x95\x8e\x8c\x2f\x82\xcc\x20\x8d\x82\x2f\x8c\x60\x97\x65\x8e\x8c\x2f\x82\xbd\x82\xa9\x20\x82\xa2\x2f\x8c\xea\x94\xf6\x2f\x82\xa2\x20\x93\xfc\x97\xcd\x2f\x96\xbc\x8e\x8c\x2f\x82\xc9\x82\xe3\x82\xa4\x82\xe8\x82\xe5\x82\xad\x20\x82\xc5\x2f\x8f\x95\x93\xae\x8e\x8c\x2f\x82\xc5\x20\x82\xb7\x2f\x8c\xea\x94\xf6\x2f\x82\xb7\x20\x81\x42\x2f\x95\xe2\x8f\x95\x8b\x4c\x8d\x86\x2f\x81\x42\n";
        // Read in a partially annotated sentence
        stringstream instr;
        instr << confident_text;
        FullCorpusIO infcio(util, instr, false);
        KyteaSentence * sent = infcio.readSentence();
        int ret = 1;
        if(sent->words.size() != 11) {
            cerr << "Did not get expected sentence size of 11: " << sent->words.size() << endl;
            ret = 0;
        } else if(sent->words[10].tags.size() != 2) {
            cerr << "Did not get two levels of tags for final word: " << sent->words[10].tags.size() << endl;
            ret = 0;
        }
        delete sent;
        return ret;
    }
    
    int testUnkIO() {
        // string input = "これ/代名詞/これ は/助詞/は 未知/名詞/みち\n";
        string input = "\x82\xb1\x82\xea\x2f\x91\xe3\x96\xbc\x8e\x8c\x2f\x82\xb1\x82\xea\x20\x82\xcd\x2f\x8f\x95\x8e\x8c\x2f\x82\xcd\x20\x96\xa2\x92\x6d\x2f\x96\xbc\x8e\x8c\x2f\x82\xdd\x82\xbf\n";
        // Read in a partially annotated sentence
        stringstream instr;
        instr << input;
        FullCorpusIO infcio(util, instr, false);
        KyteaSentence * sent = infcio.readSentence();
        sent->words[2].setUnknown(true);
        // string exp = "これ/代名詞/これ は/助詞/は 未知/名詞/みち/UNK\n";
        string exp = "\x82\xb1\x82\xea\x2f\x91\xe3\x96\xbc\x8e\x8c\x2f\x82\xb1\x82\xea\x20\x82\xcd\x2f\x8f\x95\x8e\x8c\x2f\x82\xcd\x20\x96\xa2\x92\x6d\x2f\x96\xbc\x8e\x8c\x2f\x82\xdd\x82\xbf\x2f\x55\x4e\x4b\n";
        stringstream outstr;
        FullCorpusIO outfcio(util, outstr, true);
        outfcio.setUnkTag("/UNK");
        outfcio.writeSentence(sent);
        string act = outstr.str();
        if(exp != act) {
            cerr << "exp: "<<exp<<endl<<"act: "<<act<<endl;
            return 0;
        }
        return 1;
    }

    bool runTest() {
        int done = 0, succeeded = 0;
        done++; cout << "testWordSegConf()" << endl; if(testWordSegConf()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testPartEmptyLines()" << endl; if(testPartEmptyLines()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testPartEmptyTag()" << endl; if(testPartEmptyTag()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testFullTagConf()" << endl; if(testFullTagConf()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testLastValue()" << endl; if(testLastValue()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testUnkIO()" << endl; if(testUnkIO()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestCorpusIOSjis Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

};

}



#endif
