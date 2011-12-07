/*
* Copyright 2009, KyTea Development Team
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* 
*     http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <iostream>
#include <kytea/kytea-config.h>
#include <kytea/kytea.h>

using namespace std;

namespace kytea {

class KyteaTest {

public:

    int testGetTypeString() {
        StringUtilUtf8 util;
        KyteaString str = util.mapString("漢カひ。１A");
        string act = util.getTypeString(str);
        string exp = "KTHODR";
        if(act != exp) {
            cout << "testGetTypeString::Expected "<<exp << " but got "<<act <<endl;
            return 0;
        }
        return 1;
    }
    
    int testWSNgramFeatures() {
        StringUtilUtf8 util;
        Kytea kytea;
        kytea.setWSModel(new KyteaModel());
        KyteaString str = util.mapString("漢カひ。１A");
        vector<KyteaString> exp, act;
        exp.push_back(util.mapString("X-2漢"));
        exp.push_back(util.mapString("X-1カ"));
        exp.push_back(util.mapString("X0ひ"));
        exp.push_back(util.mapString("X1。"));
        exp.push_back(util.mapString("X2１"));
        exp.push_back(util.mapString("X3A"));
        exp.push_back(util.mapString("X-2漢カ"));
        exp.push_back(util.mapString("X-1カひ"));
        exp.push_back(util.mapString("X0ひ。"));
        exp.push_back(util.mapString("X1。１"));
        exp.push_back(util.mapString("X2１A"));
        Kytea::SentenceFeatures sentFeats(5);
        vector<KyteaString> charPrefixes_;
        for(int i = -2; i <= 3; i++) {
            ostringstream oss; oss << "X" << i;
            charPrefixes_.push_back(util.mapString(oss.str()));
        }
        kytea.wsNgramFeatures(str, sentFeats, charPrefixes_, 2);
        if((int)sentFeats.size() != 5)
            THROW_ERROR("sentFeats.size() == " << sentFeats.size());
        for(int i = 0; i < (int)sentFeats[2].size(); i++)
            act.push_back(kytea.getWSModel()->showFeat(sentFeats[2][i]));
        sort(exp.begin(), exp.end());
        sort(act.begin(), act.end());
        bool bad = false;
        if(exp.size() != act.size()) {
            bad = true;
            cout << "Sizes exp.size()=="<<exp.size()<<", act.size()=="<<act.size()<<endl;
        }
        for(int i = 0; !bad && i < (int)exp.size(); i++) 
            if(exp[i] != act[i])
                bad = true;
        if(bad) {
            cout << "EXP:";
            for(int i = 0; i < (int)exp.size(); i++)
                cout << " " << util.showString(exp[i]);
            cout << endl << "ACT:";
            for(int i = 0; i < (int)act.size(); i++)
                cout << " " << util.showString(act[i]);
            cout << endl; 
        }
        return bad ? 0 : 1;
    }

    FeatureLookup * makeFeatureLookup(StringUtil * util) {
        // Make the feature values
        const int SIZE = 14;
        const char* featStrs[SIZE] = 
            { "X-2漢", "X-1カ", "X0ひ", "X1。", "X2１", "X3A",
              "X-2漢カ", "X-1カひ", "X0ひ。", "X1。１", "X2１A",
              "D0L1", "D0I5", "D1R5"};
        KyteaModel mod;
        mod.setNumClasses(2);
        mod.setLabel(0, 1);
        mod.setLabel(1, -1);
        int lastFeat = -1;
        for(int i = 0; i < SIZE; i++)
            lastFeat = mod.mapFeat(util->mapString(featStrs[i]));
        mod.initializeWeights(1, lastFeat+1);
        for(int i = 0; i < lastFeat; i++)
            mod.setWeight(i, 0, i+1);
        return mod.toFeatureLookup(util, 3, 3, 2, 5);
    }

    int testModelToLookup() {
        // Make the expected dictionary
        StringUtilUtf8 util;
        const int SIZE = 11;
        const char* wordStrs[SIZE] = { "漢", "カ", "ひ", "。", "１", "A",
                                       "漢カ", "カひ", "ひ。", "。１", "１A"};
        const int wordPoss[SIZE] = { 5, 4, 3, 2, 1, 0, 4, 3, 2, 1, 0 };
        typedef Dictionary<vector<FeatVal> >::WordMap WordMap;
        WordMap wm;
        for(int i = 0; i < SIZE; i++) {
            pair<WordMap::iterator, bool> it = wm.insert(WordMap::value_type(util.mapString(wordStrs[i]),new vector<FeatVal>(6,0)));
            (*it.first->second)[wordPoss[i]] = i+1; // Add one because first feature is NULL
        }
        Dictionary<vector<FeatVal> > exp(&util);
        exp.buildIndex(wm);
        // Convert the model to a feature lookup
        FeatureLookup * look = makeFeatureLookup(&util);
        // Check the n-gram values
        const Dictionary<vector<FeatVal> > * act = look->getCharDict();
        int ret = 1;
        if((int)act->getEntries().size() != SIZE) {
            cerr << "act->getEntries().size() == "<<act->getEntries().size()<<endl;
            ret = 0;
        } else {
            for(int i = 0; i < SIZE; i++) {
                const vector<FeatVal> * actVec = act->findEntry(util.mapString(wordStrs[i]));
                vector<FeatVal> * expVec = exp.findEntry(util.mapString(wordStrs[i]));
                if(actVec == NULL) {
                    cerr << "actVec["<<i<<"] == NULL"<<endl;
                    ret = 0;
                }
                else if(expVec == NULL) {
                    cerr << "expVec["<<i<<"] == NULL"<<endl;
                    ret = 0;
                }
                else if(*actVec != *expVec) {
                    cerr << "expVec["<<i<<"] != actVec["<<i<<"]"<<endl;
                    for(int j = 0; j < (int)expVec->size(); j++) cerr << (*expVec)[j] << " "; cerr << endl;
                    for(int j = 0; j < (int)actVec->size(); j++) cerr << (*actVec)[j] << " "; cerr << endl;
                    ret = 0;
                }
            }
        }
        // Check the dictionary match
        vector<FeatVal> dictExp(2*5*3, 0);
        dictExp[0*15+0*3+2] = SIZE+1;
        dictExp[0*15+4*3+1] = SIZE+2;
        dictExp[1*15+4*3+0] = SIZE+3;
        const vector<FeatVal> & dictAct = *look->getDictVector();
        if(dictExp.size() != dictAct.size()) {
            cerr << "dictExp.size() == "<<dictExp.size()
                 << "dictAct.size() == "<<dictAct.size() <<endl;
            ret = 0;
        } else {
            for(int i = 0; i < (int)dictAct.size(); i++) {
                if(dictExp[i] != dictAct[i]) {
                    cerr 
                        << "dictExp["<<i<<"]="<<dictExp[i]
                        << " dictAct["<<i<<"]="<<dictAct[i] << endl;
                    ret = 0;
                }
            }
        }
        delete look;
        return ret;
    }

    int testFeatureLookup() {
        StringUtilUtf8 util;
        FeatureLookup * feat = makeFeatureLookup(&util);
        KyteaString str = util.mapString("漢カひ。１A");
        vector<FeatSum> act(5,0);
        feat->addNgramScores(feat->getCharDict(), str, 3, act);
        vector<FeatSum> exp(5,0);
        exp[2] = 11*(11+1)/2; // All features from 1-11 should fire
        int ret = 1;
        for(int i = 0; i < 5; i++) {
            if(act[i] != exp[i]) {
                cerr 
                    << "act["<<i<<"]="<<act[i]
                    << " exp["<<i<<"]="<<exp[i] <<endl;
                ret = 0;
            }
        }
        return ret;
    }

    void makePrefixes(vector<KyteaString> & charPrefixes, vector<KyteaString> & typePrefixes, StringUtil * util) {
        charPrefixes.resize(0);
        for(int i = 1; i <= 2*3; i++) {
            ostringstream buff;
            buff << "X" << i-3;
            charPrefixes.push_back(util->mapString(buff.str()));
        }
        typePrefixes.resize(0);
        for(int i = 1; i <= 2*3; i++) {
            ostringstream buff;
            buff << "T" << i-3;
            typePrefixes.push_back(util->mapString(buff.str()));
        }
    }

    int testFeatureLookupMatchesModel() {
        // Make the full model
        StringUtilUtf8 util;
        Kytea kytea;
        kytea.setWSModel(new KyteaModel());
        KyteaModel & mod = *kytea.getWSModel();
        mod.setNumClasses(2);
        mod.setLabel(0, 1);
        mod.setLabel(1, -1);
        mod.setNumWeights(1);
        int id = 0;
        KyteaString str = util.mapString("漢カひ。１A");
        KyteaString typeStr = util.mapString(util.getTypeString(str));
        for(int i = 0; i < (int)str.length(); i++) {
            for(int j = 1; j <= 3; j++) {
                if(i+j > (int)str.length()) break;
                for(int k = -2; k <= 4-j; k++) {
                    ostringstream oss1; oss1 << "X" << k << util.showString(str.substr(i,j));
                    id = mod.mapFeat(util.mapString(oss1.str()));
                    ostringstream oss2; oss2 << "T" << k << util.showString(typeStr.substr(i,j));
                    id = mod.mapFeat(util.mapString(oss2.str()));
                }
            }
        }
        mod.initializeWeights(1, id+1);
        for(int i = 0; i <= id; i++) {
            mod.setWeight(i, 0, i);
        }
        // Make the feature lookup
        FeatureLookup * feat = mod.toFeatureLookup(&util, 3, 3, 2, 5);
        // Get the score matrix for lookup
        vector<FeatSum> act(5,0);
        feat->addNgramScores(feat->getCharDict(), str, 3, act);
        feat->addNgramScores(feat->getTypeDict(), typeStr, 3, act);
        for(int i = 0; i < 5; i++)
            act[i] += feat->getBias();
        // Calculate the n-gram features
        Kytea::SentenceFeatures sentFeats(5);
        vector<KyteaString> charPrefixes, typePrefixes;
        makePrefixes(charPrefixes, typePrefixes, &util);
        kytea.wsNgramFeatures(str, sentFeats, charPrefixes, 3);
        kytea.wsNgramFeatures(typeStr, sentFeats, typePrefixes, 3);
        int ret = 1;
        for(int i = 0; i < 5; i++) {
            pair<int,double> answer = mod.runClassifier(sentFeats[i])[0];
            if(answer.second != act[i]) {
                cerr << "model["<<i<<"]="<<answer.second<<", act["<<i<<"]="<<act[i]<<endl;
                ret = 0;
            }
        }
        delete feat;
        return ret;
    }

    int testFeatureLookupDictionary() {
        StringUtilUtf8 util;
        FeatureLookup * look = makeFeatureLookup(&util);
        KyteaString str = util.mapString("漢カひ。１A");
        Kytea kytea;
        // Dictionary
        Dictionary<ModelTagEntry>::WordMap dictMap;
        kytea.addTag<ModelTagEntry>(dictMap, util.mapString("１"), 0, NULL, 0);
        kytea.addTag<ModelTagEntry>(dictMap, util.mapString("漢カひ。１A"), 0, NULL, 0);
        kytea.addTag<ModelTagEntry>(dictMap, util.mapString("漢カひ。１"), 0, NULL, 0);
        kytea.addTag<ModelTagEntry>(dictMap, util.mapString("カひ。１A"), 0, NULL, 1);
        Dictionary<ModelTagEntry> dict(&util);
        dict.buildIndex(dictMap);
        // Check that these are added correctly
        vector<FeatSum> exp(5,13); // All are inside D1I5 (only once)
        exp[4] += 12; // The last one is to the right of D1L1
        exp[0] += 14; // The last one is to the right of D2R5
        vector<FeatSum> act(5,0);
        look->addDictionaryScores(dict.match(str), 2, 5, act);
        // Check that these are equal
        int ret = 1;
        for(int i = 0; i < 5; i++) {
            if(act[i] != exp[i]) {
                cerr 
                    << "act["<<i<<"]="<<act[i]
                    << " exp["<<i<<"]="<<exp[i] <<endl;
                ret = 0;
            }
        }
        return ret;
    }

    void runTest() {
        int done = 0, succeeded = 0;
        done++; cout << "testGetTypeString()" << endl; if(testGetTypeString()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testWSNgramFeatures()" << endl; if(testWSNgramFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testModelToLookup()" << endl; if(testModelToLookup()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testFeatureLookup()" << endl; if(testFeatureLookup()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testFeatureLookupMatchesModel()" << endl; if(testFeatureLookupMatchesModel()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "testFeatureLookupDictionary()" << endl; if(testFeatureLookupDictionary()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "Finished with "<<succeeded<<"/"<<done<<" tests succeeding"<<endl;
    }

};

}

int main(int argv, char **argc) {
    kytea::KyteaTest tests;
    tests.runTest();
}
