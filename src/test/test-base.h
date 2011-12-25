#ifndef TEST_BASE__
#define TEST_BASE__

namespace kytea {

class TestBase {

protected:

    int checkWordSeg(const KyteaSentence & sent, const vector<KyteaString> & toks, StringUtil * util) {
        const KyteaSentence::Words & words =  sent.words;
        int ok = 1;
        for(int i = 0; i < (int)max(words.size(), toks.size()); i++) {
            if(i >= (int)words.size() || i >= (int)toks.size() || words[i].surf != toks[i]) {
                ok = 0;
                cout << "words["<<i<<"] != toks["<<i<<"] ("<<
                    (i >= (int)words.size() ? "NULL" : util->showString(words[i].surf))
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
