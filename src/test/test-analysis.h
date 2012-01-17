#ifndef TEST_ANALYSIS__
#define TEST_ANALYSIS__

#include "test-base.h"
#include <kytea/corpus-io.h>
#include <kytea/model-io.h>

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
        config->setOnTraining(false);
    }

    ~TestAnalysis() {
        delete kytea;
    }

    int testWordSegmentation() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaSentence sentence(util->mapString("これは学習データです。"));
        kytea->calculateWS(sentence);
        // Make the correct words
        KyteaString::Tokens toks = util->mapString("これ は 学習 データ で す 。").tokenize(util->mapString(" "));
        return checkWordSeg(sentence,toks,util);
    }

    int testGlobalTagging() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaSentence sentence(util->mapString("これは学習データです。"));
        kytea->calculateWS(sentence);
        kytea->calculateTags(sentence,0);
        // Make the correct tags
        KyteaString::Tokens toks = util->mapString("代名詞 助詞 名詞 名詞 助動詞 語尾 補助記号").tokenize(util->mapString(" "));
        return checkTags(sentence,toks,0,util);
    }

    int testGlobalSelf() {
        KyteaString::Tokens words = util->mapString("これ は 学習 データ どうぞ 。").tokenize(util->mapString(" "));
        KyteaString::Tokens tags = util->mapString("代名詞 助詞 名詞 名詞 副詞 補助記号").tokenize(util->mapString(" "));
        KyteaString::Tokens singleTag(1);
        if(words.size() != tags.size()) THROW_ERROR("words.size() != tags.size() in testGlobalSelf");
        int ok = 1;
        for(int i = 0; i < (int)words.size(); i++) {
            KyteaSentence sent(words[i]);
            sent.refreshWS(1);
            if(sent.words.size() != 1) THROW_ERROR("Bad segmentation in testGlobalSelf");
            kytea->calculateTags(sent,0);
            singleTag[0] = tags[i];
            ok = (checkTags(sent,singleTag,0,util) ? ok : 0);
        }
        return ok;
    }

    int testLocalTagging() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaSentence sentence(util->mapString("これは学習データです。"));
        kytea->calculateWS(sentence);
        kytea->calculateTags(sentence,1);
        // Make the correct tags
        KyteaString::Tokens toks = util->mapString("これ は がくしゅう でーた で す 。").tokenize(util->mapString(" "));
        return checkTags(sentence,toks,1,util);
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

    int testConfidentInput() {
        // This is currently aborting, make sure it fails but continue
        string confident_text = "これ/代名詞/これ は/助詞/は 信頼/名詞/しんらい 度/接尾辞/ど の/助詞/の 高/形容詞/たか い/語尾/い 入力/名詞/にゅうりょく で/助動詞/で す/語尾/す 。/補助記号/。\n";
        // Read in a partially annotated sentence
        stringstream instr;
        instr << confident_text;
        FullCorpusIO infcio(util, instr, false);
        KyteaSentence * sent = infcio.readSentence();
        // Calculate the tags
        kytea->calculateWS(*sent);
        kytea->calculateTags(*sent,0);
        kytea->calculateTags(*sent,1);
        // Write out the sentence
        stringstream outstr;
        FullCorpusIO outfcio(util, outstr, true);
        outfcio.writeSentence(sent);
        string actual_text = outstr.str();
        delete sent;
        if(actual_text != confident_text) {
            cout << "actual_text != confident_text"<<endl<<" "<<actual_text<<endl<<" "<<confident_text<<endl;
            return 0;
        } else {
            return 1;
        } 
    }

    int testTextIO() {
        // Write the model
        kytea->getConfig()->setModelFormat(ModelIO::FORMAT_TEXT);
        kytea->writeModel("/tmp/kytea-model.txt");
        // Read the model
        Kytea actKytea;
        actKytea.readModel("/tmp/kytea-model.txt");
        // Check that they are equal
        kytea->checkEqual(actKytea);
        return 1;
    }

    int testBinaryIO() {
        // Write the model
        kytea->getConfig()->setModelFormat(ModelIO::FORMAT_BINARY);
        kytea->writeModel("/tmp/kytea-model.bin");
        // Read the model
        Kytea actKytea;
        actKytea.readModel("/tmp/kytea-model.bin");
        // Check that they are equal
        kytea->checkEqual(actKytea);
        return 1;
    }

    bool runTest() {
        int done = 0, succeeded = 0;
        done++; cout << "testWordSegmentation()" << endl; if(testWordSegmentation()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testGlobalTagging()" << endl; if(testGlobalTagging()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testGlobalSelf()" << endl; if(testGlobalSelf()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testLocalTagging()" << endl; if(testLocalTagging()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testPartialSegmentation()" << endl; if(testPartialSegmentation()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testTextIO()" << endl; if(testTextIO()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testBinaryIO()" << endl; if(testBinaryIO()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testConfidentInput()" << endl; if(testConfidentInput()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestAnalysis Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

};

}



#endif
