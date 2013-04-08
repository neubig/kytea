#ifndef TEST_CORPUSIO_EUC__
#define TEST_CORPUSIO_EUC__

#include <kytea/corpus-io.h>
#include "test-base.h"

namespace kytea {

class TestCorpusIOEuc : public TestBase {

private:

    StringUtilEuc * util;

public:

    TestCorpusIOEuc() {
        util = new StringUtilEuc;
    }

    ~TestCorpusIOEuc() {
        delete util;    
    }

    int testWordSegConf() {
        // Build the string
        stringstream instr;
        // instr << "こ|れ-は デ ー タ で-す 。" << endl;
        instr << "\xa4\xb3\x7c\xa4\xec\x2d\xa4\xcf\x20\xa5\xc7\x20\xa1\xbc\x20\xa5\xbf\x20\xa4\xc7\x2d\xa4\xb9\x20\xa1\xa3" << endl;
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
        instr << "\xa4\xb3\x2d\xa4\xec\x2f\x2f\xa4\xb3\xa4\xec" << endl;
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
        instr << "\xa4\xb3\x2d\xa4\xec\x2f\xcc\xbe\xbb\xec\x20\xa4\xcf\x2f\xbd\xf5\xbb\xec\x20\xa5\xc7\xa1\xbc\xa5\xbf\x2f\xcc\xbe\xbb\xec\x20\xa4\xc7\x2f\xbd\xf5\xc6\xb0\xbb\xec\x20\xa4\xb9\x2f\xb8\xec\xc8\xf8\x20\xa1\xa3\x2f\xca\xe4\xbd\xf5\xb5\xad\xb9\xe6" << endl;
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
        string confident_text = "\xa4\xb3\xa4\xec\x2f\xc2\xe5\xcc\xbe\xbb\xec\x2f\xa4\xb3\xa4\xec\x20\xa4\xcf\x2f\xbd\xf5\xbb\xec\x2f\xa4\xcf\x20\xbf\xae\xcd\xea\x2f\xcc\xbe\xbb\xec\x2f\xa4\xb7\xa4\xf3\xa4\xe9\xa4\xa4\x20\xc5\xd9\x2f\xc0\xdc\xc8\xf8\xbc\xad\x2f\xa4\xc9\x20\xa4\xce\x2f\xbd\xf5\xbb\xec\x2f\xa4\xce\x20\xb9\xe2\x2f\xb7\xc1\xcd\xc6\xbb\xec\x2f\xa4\xbf\xa4\xab\x20\xa4\xa4\x2f\xb8\xec\xc8\xf8\x2f\xa4\xa4\x20\xc6\xfe\xce\xcf\x2f\xcc\xbe\xbb\xec\x2f\xa4\xcb\xa4\xe5\xa4\xa6\xa4\xea\xa4\xe7\xa4\xaf\x20\xa4\xc7\x2f\xbd\xf5\xc6\xb0\xbb\xec\x2f\xa4\xc7\x20\xa4\xb9\x2f\xb8\xec\xc8\xf8\x2f\xa4\xb9\x20\xa1\xa3\x2f\xca\xe4\xbd\xf5\xb5\xad\xb9\xe6\x2f\xa1\xa3\n";
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
        string input = "\xa4\xb3\xa4\xec\x2f\xc2\xe5\xcc\xbe\xbb\xec\x2f\xa4\xb3\xa4\xec\x20\xa4\xcf\x2f\xbd\xf5\xbb\xec\x2f\xa4\xcf\x20\xcc\xa4\xc3\xce\x2f\xcc\xbe\xbb\xec\x2f\xa4\xdf\xa4\xc1\n";
        // Read in a partially annotated sentence
        stringstream instr;
        instr << input;
        FullCorpusIO infcio(util, instr, false);
        KyteaSentence * sent = infcio.readSentence();
        sent->words[2].setUnknown(true);
        // string exp = "これ/代名詞/これ は/助詞/は 未知/名詞/みち/UNK\n";
        string exp = "\xa4\xb3\xa4\xec\x2f\xc2\xe5\xcc\xbe\xbb\xec\x2f\xa4\xb3\xa4\xec\x20\xa4\xcf\x2f\xbd\xf5\xbb\xec\x2f\xa4\xcf\x20\xcc\xa4\xc3\xce\x2f\xcc\xbe\xbb\xec\x2f\xa4\xdf\xa4\xc1\x2f\x55\x4e\x4b\n";
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
        cout << "#### TestCorpusIOEuc Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

};

}



#endif
