#ifndef TEST_BASE__
#define TEST_BASE__

namespace kytea {

class TestBase {

protected:

    TestBase() : passed_(false) { }

    bool passed_;

    int checkWordSeg(const KyteaSentence & sent, const vector<KyteaString> & toks, StringUtil * util) {
        const KyteaSentence::Words & words =  sent.words;
        int ok = 1;
        for(int i = 0; i < (int)max(words.size(), toks.size()); i++) {
            if(i >= (int)words.size() || i >= (int)toks.size() || words[i].surface != toks[i]) {
                ok = 0;
                cout << "words["<<i<<"] != toks["<<i<<"] ("<<
                    (i >= (int)words.size() ? "NULL" : util->showString(words[i].surface))
                    <<" != "<<
                    (i >= (int)toks.size() ? "NULL" : util->showString(toks[i]))
                    <<")"<<endl;
            }
        }
        return ok;
    }
    
    int checkTags(const KyteaSentence & sent, const vector<KyteaString> & toks, int pos, StringUtil * util) {
        const KyteaSentence::Words & words =  sent.words;
        int ok = (words.size() == toks.size() ? 1 : 0);
        KyteaString noneString = util->mapString("NONE");
        for(int i = 0; i < (int)max(words.size(), toks.size()); i++) {
            // Find the proper tag
            KyteaString myTag;
            if(i >= (int)words.size())
                myTag = util->mapString("NULL");
            else if(pos >= (int)words[i].tags.size() || 0 == (int)words[i].tags[pos].size())
                myTag = util->mapString("NONE");
            else
                myTag = words[i].tags[pos][0].first;
            // If they don't match return
            if(i >= (int)toks.size() || myTag != toks[i]) {
                ok = 0;
                cout << "words["<<i<<"] != toks["<<i<<"] ("<<
                    util->showString(myTag)
                    <<" != "<<
                    (i >= (int)toks.size() ? "NULL" : util->showString(toks[i]))
                    <<")"<<endl;
            }
        }
        return ok;
    }


    template<class T>
    int checkVector(const vector<T> & exp, const vector<T> & act) {
        int ok = 1;
        for(int i = 0; i < (int)max(exp.size(), act.size()); i++) {
            if(i >= (int)exp.size() || i >= (int)act.size() || exp[i] != act[i]) {
                ok = 0;
                cout << "exp["<<i<<"] != act["<<i<<"] (";
                if(i >= (int)exp.size()) cout << "NULL"; else cout << exp[i];
                cout <<" != ";
                if(i >= (int)act.size()) cout << "NULL"; else cout << act[i];
                cout << ")" << endl;
            }
        }
        return ok;
    }

};

}

#endif
