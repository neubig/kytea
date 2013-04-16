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

    int testTokReadSentence() {
        stringstream instr;
        instr << "これ は 学習 データ で す 。" << endl;
        TokenizedCorpusIO io(util, instr, false);
        KyteaSentence * sent = io.readSentence();
        // Make the correct words
        KyteaString::Tokens toks = util->mapString("これ は 学習 データ で す 。").tokenize(util->mapString(" "));
        return checkWordSeg(*sent,toks,util);
    }

    int testRawReadSlash() {
        stringstream instr;
        instr << "右/左" << endl;
        RawCorpusIO io(util, instr, false);
        KyteaSentence * sent = io.readSentence();
        // Make the correct words
        KyteaString exp = util->mapString("右/左");
        return exp == sent->surface;
    }

    int testPartEmptyTag() {
        // Build the string
        stringstream instr;
        instr << "リ-リ-カ-ル//りりかる|な-の-は//なのは|を 中 心 に 、" << endl;
        PartCorpusIO io(util, instr, false);
        KyteaSentence * sent = io.readSentence();
        int ret = 1;
        if(sent->words.size() != 3) {
            cerr << "Sentence size " << sent->words.size() << " != 3" << endl;
            ret = 0;
        }
        if(sent->words[0].tags.size() != 2) {
            cerr << "Words[0] tags " << sent->words[0].tags.size() << " != 2" << endl;
            ret = 0;
        }
        if(sent->words[1].tags.size() != 2) {
            cerr << "Words[1] tags " << sent->words[1].tags.size() << " != 2" << endl;
            ret = 0;
        }
        delete sent;
        return ret;
    }

    int testTagIO() {
        // Build the string
        stringstream instr, outstr;
        instr << "こ-れ/名詞 は/助詞 データ/名詞 で/助動詞 す/語尾 。/補助記号" << endl;
        FullCorpusIO ioin(util, instr, false);
        KyteaSentence * sent = ioin.readSentence();
        FullCorpusIO ioout(util, outstr, true);
        ioout.setPrintWords(false);
        ioout.writeSentence(sent);
        string exp = "名詞 助詞 名詞 助動詞 語尾 補助記号\n";
        string act = outstr.str();
        delete sent;
        if(exp != act) {
            cerr << exp << endl << act << endl;
            return 0;
        } else {
            return 1;
        }
    }

    int testFullTagConf() {
        // Build the string
        stringstream instr;
        instr << "こ-れ/名詞 は/助詞 データ/名詞 で/助動詞 す/語尾 。/補助記号" << endl;
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
        string confident_text = "これ/代名詞/これ は/助詞/は 信頼/名詞/しんらい 度/接尾辞/ど の/助詞/の 高/形容詞/たか い/語尾/い 入力/名詞/にゅうりょく で/助動詞/で す/語尾/す 。/補助記号/。\n";
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
        string input = "これ/代名詞/これ は/助詞/は 未知/名詞/みち\n";
        // Read in a partially annotated sentence
        stringstream instr;
        instr << input;
        FullCorpusIO infcio(util, instr, false);
        KyteaSentence * sent = infcio.readSentence();
        sent->words[2].setUnknown(true);
        string exp = "これ/代名詞/これ は/助詞/は 未知/名詞/みち/UNK\n";
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
        done++; cout << "testTagIO()" << endl; if(testTagIO()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testTokReadSentence()" << endl; if(testTokReadSentence()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testRawReadSlash()" << endl; if(testRawReadSlash()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestCorpusIO Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

};

}



#endif
