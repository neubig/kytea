#ifndef TEST_ANALYSIS__
#define TEST_ANALYSIS__

#include <cmath>
#include "test-base.h"

namespace kytea {

class TestAnalysis : public TestBase {

private:

    Kytea *kytea, *kyteaLogist, *kyteaMCSVM, *kyteaNoWS;
    StringUtil *util, *utilLogist, *utilMCSVM, *utilNoWS;

public:

    TestAnalysis() {
        // Print the corpus
        const char* toy_text = 
"これ/代名詞/これ は/助詞/は 学習/名詞/がくしゅう データ/名詞/でーた で/助動詞/で す/語尾/す 。/補助記号/。\n"
"大変/形状詞/でーた で/助動詞/で す/語尾/す 。/補助記号/。\n"
"\n"
"どうぞ/副詞/どうぞ モデル/名詞/もでる を/助詞/を ＫｙＴｅａ/名詞/きゅーてぃー で/助詞/で 学習/名詞/がくしゅう し/動詞/し て/助詞/て くださ/動詞/くださ い/語尾/い ！/補助記号/！\n"
"処理/名詞/しょり を/助詞/を 行/動詞/おこな っ/語尾/っ た/助動詞/た ．/補助記号/。\n"
"京都/名詞/きょうと に/助詞/に 行/動詞/い っ/語尾/っ た/助動詞/た ．/補助記号/。\n";
        ofstream ofs("/tmp/kytea-toy-corpus.txt"); 
        ofs << toy_text; ofs.close();
        // Train the KyTea model with SVMs
        const char* toyCmd[7] = {"", "-model", "/tmp/kytea-svm-model.bin", "-full", "/tmp/kytea-toy-corpus.txt", "-global", "1"};
        KyteaConfig * config = new KyteaConfig;
        config->setDebug(0);
        config->setOnTraining(true);
        config->parseTrainCommandLine(7, toyCmd);
        kytea = new Kytea(config);
        kytea->trainAll();
        util = kytea->getStringUtil();
        config->setOnTraining(false);
        // Train the KyTea model with logistic regression
        const char* toyCmdLogist[9] = {"", "-model", "/tmp/kytea-logist-model.bin", "-full", "/tmp/kytea-toy-corpus.txt", "-global", "1", "-solver", "0"};
        KyteaConfig * configLogist = new KyteaConfig;
        configLogist->setDebug(0);
        configLogist->setTagMax(0);
        configLogist->setOnTraining(true);
        configLogist->parseTrainCommandLine(9, toyCmdLogist);
        kyteaLogist = new Kytea(configLogist);
        kyteaLogist->trainAll();
        utilLogist = kyteaLogist->getStringUtil();
        configLogist->setOnTraining(false);
        // Train the KyTea model with the multi-class svm
        const char* toyCmdMCSVM[9] = {"", "-model", "/tmp/kytea-logist-model.bin", "-full", "/tmp/kytea-toy-corpus.txt", "-global", "1", "-solver", "4"};
        KyteaConfig * configMCSVM = new KyteaConfig;
        configMCSVM->setDebug(0);
        configMCSVM->setTagMax(0);
        configMCSVM->setOnTraining(true);
        configMCSVM->parseTrainCommandLine(9, toyCmdMCSVM);
        kyteaMCSVM = new Kytea(configMCSVM);
        kyteaMCSVM->trainAll();
        utilMCSVM = kyteaMCSVM->getStringUtil();
        configMCSVM->setOnTraining(false);
        // Train the KyTea model with logistic regression
        const char* toyCmdNoWS[8] = {"", "-model", "/tmp/kytea-logist-model.bin", "-full", "/tmp/kytea-toy-corpus.txt", "-global", "1", "-nows"};
        KyteaConfig * configNoWS = new KyteaConfig;
        configNoWS->setDebug(0);
        configNoWS->setTagMax(0);
        configNoWS->setOnTraining(true);
        configNoWS->parseTrainCommandLine(8, toyCmdNoWS);
        kyteaNoWS = new Kytea(configNoWS);
        kyteaNoWS->trainAll();
        utilNoWS = kyteaNoWS->getStringUtil();
        configNoWS->setOnTraining(false);
    }

    ~TestAnalysis() {
        delete kytea;
        delete kyteaLogist;
        delete kyteaMCSVM;
        delete kyteaNoWS;
    }

    int testWordSegmentationEmpty() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = util->mapString("");
        KyteaSentence sentence(str, util->normalize(str));
        kytea->calculateWS(sentence);
        // Make the correct words
        KyteaString::Tokens toks = util->mapString("").tokenize(util->mapString(" "));
        return checkWordSeg(sentence,toks,util);
    }

    int testWordSegmentationUnk() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = util->mapString("これは学習デエタです。");
        KyteaSentence sentence(str, util->normalize(str));
        kytea->calculateWS(sentence);
        // Make the correct words
        KyteaString::Tokens toks = util->mapString("これ は 学習 デエタ で す 。").tokenize(util->mapString(" "));
        if(!checkWordSeg(sentence,toks,util)) { return 0; }
        vector<bool> unk_exp(6, false), unk_act(6);
        unk_exp[3] = true;
        for(int i = 0; i < 6; i++)
            unk_act[i] = sentence.words[i].getUnknown();
        return checkVector(unk_exp, unk_act);
    }
    
    int testNormalizationUnk() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = util->mapString("これはKyTeaです.");
        KyteaSentence sentence(str, util->normalize(str));
        kytea->calculateWS(sentence);
        // Make the correct words
        KyteaString::Tokens toks = util->mapString("これ は KyTea で す .").tokenize(util->mapString(" "));
        if(!checkWordSeg(sentence,toks,util)) { return 0; }
        vector<bool> unk_exp(6, false), unk_act(6);
        for(int i = 0; i < 6; i++)
            unk_act[i] = sentence.words[i].getUnknown();
        return checkVector(unk_exp, unk_act);
    }

    int testWordSegmentationSVM() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = util->mapString("これは学習データです。");
        KyteaSentence sentence(str, util->normalize(str));
        kytea->calculateWS(sentence);
        // Make the correct words
        KyteaString::Tokens toks = util->mapString("これ は 学習 データ で す 。").tokenize(util->mapString(" "));
        return checkWordSeg(sentence,toks,util);
    }

    int testWordSegmentationMCSVM() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = utilMCSVM->mapString("これは学習データです。");
        KyteaSentence sentence(str, utilMCSVM->normalize(str));
        kyteaMCSVM->calculateWS(sentence);
        // Make the correct words
        KyteaString::Tokens toks = utilMCSVM->mapString("これ は 学習 データ で す 。").tokenize(utilMCSVM->mapString(" "));
        return checkWordSeg(sentence,toks,utilMCSVM);
    }

    int testWordSegmentationLogistic() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = utilLogist->mapString("これは学習データです。");
        KyteaSentence sentence(str, utilLogist->normalize(str));
        kyteaLogist->calculateWS(sentence);
        // Make the correct words
        KyteaString::Tokens toks = utilLogist->mapString("これ は 学習 データ で す 。").tokenize(utilLogist->mapString(" "));
        int correct = checkWordSeg(sentence,toks,utilLogist);
        if(correct) {
            for(int i = 0; i < (int)sentence.wsConfs.size(); i++) {
                if(sentence.wsConfs[i] < 0.0 || sentence.wsConfs[i] > 1.0) {
                    cerr << "Confidience for logistic WS "<<i<<" is not probability: " << sentence.wsConfs[i] << endl;
                    correct = 0;
                }
            }
        }
        return correct;
    }

    int testGlobalTaggingSVM() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = util->mapString("これは学習データです。");
        KyteaSentence sentence(str, util->normalize(str));
        kytea->calculateWS(sentence);
        kytea->calculateTags(sentence,0);
        // Make the correct tags
        KyteaString::Tokens toks = util->mapString("代名詞 助詞 名詞 名詞 助動詞 語尾 補助記号").tokenize(util->mapString(" "));
        int correct = checkTags(sentence,toks,0,util);
        if(correct) {
            // Check the confidences for the SVM, the second candidate should
            // always be zero
            for(int i = 0; i < (int)sentence.words.size(); i++) {
                if(sentence.words[i].tags[0][1].second != 0.0) {
                    cerr << "Margin on word "<<i<<" is not 0.0 (== "<<sentence.words[i].tags[0][1].second<<")"<<endl;
                    correct = false;
                }
            }
        }
        return correct;
    }

    int testGlobalTaggingMCSVM() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = utilMCSVM->mapString("これは学習データです。");
        KyteaSentence sentence(str, utilMCSVM->normalize(str));
        kyteaMCSVM->calculateWS(sentence);
        kyteaMCSVM->calculateTags(sentence,0);
        // Make the correct tags
        KyteaString::Tokens toks = utilMCSVM->mapString("代名詞 助詞 名詞 名詞 助動詞 語尾 補助記号").tokenize(utilMCSVM->mapString(" "));
        int correct = checkTags(sentence,toks,0,utilMCSVM);
        if(correct) {
            // Check the confidences for the SVM, the second candidate should
            // always be zero
            for(int i = 0; i < (int)sentence.words.size(); i++) {
                if(sentence.words[i].tags[0][1].second != 0.0) {
                    cerr << "Margin on word "<<i<<" is not 0.0 (== "<<sentence.words[i].tags[0][1].second<<")"<<endl;
                    correct = false;
                }
            }
        }
        return correct;
    }

    int testGlobalTaggingNoWS() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = utilNoWS->mapString("これは学習データです。");
        KyteaSentence sentence(str, utilNoWS->normalize(str));
        sentence.wsConfs[0] = -1; sentence.wsConfs[1] = 1; sentence.wsConfs[2] = 1;
        sentence.wsConfs[3] = -1; sentence.wsConfs[4] = 1; sentence.wsConfs[5] = -1;
        sentence.wsConfs[6] = -1; sentence.wsConfs[7] = 1; sentence.wsConfs[8] = 1; sentence.wsConfs[9] = 1;
        sentence.refreshWS(0);
        kyteaNoWS->calculateTags(sentence,0);
        // Make the correct tags
        KyteaString::Tokens toks = utilNoWS->mapString("代名詞 助詞 名詞 名詞 助動詞 語尾 補助記号").tokenize(utilNoWS->mapString(" "));
        int correct = checkTags(sentence,toks,0,utilNoWS);
        if(correct) {
            // Check the confidences for the SVM, the second candidate should
            // always be zero
            for(int i = 0; i < (int)sentence.words.size(); i++) {
                if(sentence.words[i].tags[0][1].second != 0.0) {
                    cerr << "Margin on word "<<i<<" is not 0.0 (== "<<sentence.words[i].tags[0][1].second<<")"<<endl;
                    correct = false;
                }
            }
        }
        return correct;
    }

    int testNoWSUnk() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = utilNoWS->mapString("これは学習デエタです。");
        KyteaSentence sentence(str, utilNoWS->normalize(str));
        sentence.wsConfs[0] = -1; sentence.wsConfs[1] = 1;
        sentence.wsConfs[2] = 1; sentence.wsConfs[3] = -1; 
        sentence.wsConfs[4] = 1; sentence.wsConfs[5] = -1;
        sentence.wsConfs[6] = -1; sentence.wsConfs[7] = 1;
        sentence.wsConfs[8] = 1; sentence.wsConfs[9] = 1;
        sentence.refreshWS(0);
        for(int i = 0; i < 7; i++)
            sentence.words[i].setUnknown(false);
        kyteaNoWS->calculateTags(sentence,0);
        // Check to make sure unknown is correct
        vector<bool> unk_exp(7, false), unk_act(7);
        unk_exp[3] = true;
        for(int i = 0; i < 7; i++)
            unk_act[i] = sentence.words[i].getUnknown();
        return checkVector(unk_exp, unk_act);
    }

    int testGlobalTaggingLogistic() {
        // Do the analysis (This is very close to the training data, so it
        // should work perfectly)
        KyteaString str = utilLogist->mapString("これは学習データです。");
        KyteaSentence sentence(str, utilLogist->normalize(str));
        kyteaLogist->calculateWS(sentence);
        kyteaLogist->calculateTags(sentence,0);
        // Make the correct tags
        KyteaString::Tokens toks = utilLogist->mapString("代名詞 助詞 名詞 名詞 助動詞 語尾 補助記号").tokenize(util->mapString(" "));
        int correct = checkTags(sentence,toks,0,utilLogist);
        if(correct) {
            // Check the confidences for the SVM, the second candidate should
            // always be zero
            for(int i = 0; i < (int)sentence.words.size(); i++) {
                double sum = 0.0;
                for(int j = 0; j < (int)sentence.words[i].tags[0].size(); j++)
                    sum += sentence.words[i].tags[0][j].second;
                if(fabs(1.0-sum) > 0.01) {
                    cerr << "Probability on word "<<i<<" is not close to 1 (== "<<sum<<")"<<endl;
                    correct = false;
                }
            }
        }
        return correct;
    }

    int testGlobalSelf() {
        KyteaString::Tokens words = util->mapString("これ 京都 学習 データ どうぞ 。").tokenize(util->mapString(" "));
        KyteaString::Tokens tags = util->mapString("代名詞 名詞 名詞 名詞 副詞 補助記号").tokenize(util->mapString(" "));
        KyteaString::Tokens singleTag(1);
        if(words.size() != tags.size()) THROW_ERROR("words.size() != tags.size() in testGlobalSelf");
        int ok = 1;
        for(int i = 0; i < (int)words.size(); i++) {
            KyteaSentence sent(words[i], util->normalize(words[i]));
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
        KyteaString str = util->mapString("東京に行った。");
        KyteaSentence sentence(str, util->normalize(str));
        kytea->calculateWS(sentence);
        kytea->calculateTags(sentence,1);
        // Make the correct tags
        KyteaString::Tokens toks = util->mapString("UNK に い っ た 。").tokenize(util->mapString(" "));
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
        string confident_text = "これ/代名詞/これ は/助詞/は 信頼/名詞/しんらい 度/接尾辞/ど の/助詞/の 高/形容詞/たか い/語尾/い 入力/名詞/にゅうりょく で/助動詞/で す/語尾/す 。/補助記号/。\n";
        // Read in a partially annotated sentence
        stringstream instr;
        instr << confident_text;
        FullCorpusIO infcio(util, instr, false);
        KyteaSentence * sent = infcio.readSentence();
        // Calculate the WS
        kytea->calculateWS(*sent);
        // Write out the sentence
        stringstream outstr1;
        FullCorpusIO outfcio1(util, outstr1, true);
        outfcio1.writeSentence(sent);
        string actual_text = outstr1.str();
        if(actual_text != confident_text) {
            cout << "WS: actual_text != confident_text"<<endl<<" "<<actual_text<<endl<<" "<<confident_text<<endl;
            return 0;
        }
        // Calculate the tags
        kytea->calculateTags(*sent,0);
        kytea->calculateTags(*sent,1);
        // Write out the sentence
        stringstream outstr2;
        FullCorpusIO outfcio2(util, outstr2, true);
        outfcio2.writeSentence(sent);
        actual_text = outstr2.str();
        delete sent;
        if(actual_text != confident_text) {
            cout << "Tag: actual_text != confident_text"<<endl<<" "<<actual_text<<endl<<" "<<confident_text<<endl;
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
        done++; cout << "testWordSegmentationSVM()" << endl; if(testWordSegmentationSVM()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testWordSegmentationEmpty()" << endl; if(testWordSegmentationEmpty()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testWordSegmentationUnk()" << endl; if(testWordSegmentationUnk()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testWordSegmentationLogistic()" << endl; if(testWordSegmentationLogistic()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testGlobalTaggingSVM()" << endl; if(testGlobalTaggingSVM()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testGlobalTaggingLogistic()" << endl; if(testGlobalTaggingLogistic()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testGlobalTaggingNoWS()" << endl; if(testGlobalTaggingNoWS()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testNoWSUnk()" << endl; if(testNoWSUnk()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testGlobalSelf()" << endl; if(testGlobalSelf()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testNormalizationUnk()" << endl; if(testNormalizationUnk()) succeeded++; else cout << "FAILED!!!" << endl;
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
